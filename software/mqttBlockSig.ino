// ------------------------------------------------------------------------------------------------------------------------------
//
//    MQTT Block signal v1.0.0
//    copyright (c) Benny Tjäder
//
//    Derivated from MMRC client made by Peter Kindström
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published
//    by the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Uses and have been tested with the following libraries
//      ArduinoHttpClient v0.4.0  by                     - https://github.com/arduino-libraries/ArduinoHttpClient
//      ArduinoJson       v6.19.3 by Benoit Blanchon     - https://github.com/bblanchon/ArduinoJson
//      ButtonDebounce    v1.0.1  by Maykon L. Capellari - https://github.com/maykon/ButtonDebounce
//      FastLed           v3.3.3  by Daniel Garcia       - https://github.com/FastLED/FastLED
//      IotWebConf        v3.2.0  by Balazs Kelemen      - https://github.com/prampec/IotWebConf
//      PubSubClient      v2.8.0  by Nick O'Leary        - https://pubsubclient.knolleary.net/
//
//    This program is realizing a block signal module for single track.
//    The signal module has one IR modules for block detection.
//    The signal module also has two signals, one in each direction. The signals supported
//    are from two lights (Hsi2) up to five lights (Hsi5) main signal or two-three lights
//    distance signals (Fsi2, Fsi3).
//
//    Signal  aspects
//    Hsi2    stop, d80, -  , -       , -      , -      , -
//    Hsi3    stop, d80, d40, -       , -      , -      , -
//    Hsi4    stop, d80, d40, d80wstop, d80wd80, -      , -
//    Hsi5    stop, d80, d40, d80wstop, d80wd80, d80wd40, d40short
//    Fsi2    -   , -  , -  , d80wstop, d80wd80, -      , -
//    Fsi3    -   , -  , -  , d80wstop, d80wd80, d80wd40, -
//
//    The signals acts on the two surrounding block signal modules, one upwards (forward) and one downwards (backward).
//
// ------------------------------------------------------------------------------------------------------------------------------
// Include all libraries
#include <ArduinoHttpClient.h>                    // Library to handle HTTP download of config file
#include <ArduinoJson.h>                          // Library to handle JSON for the config file
#include <PubSubClient.h>                         // Library to handle MQTT communication
#include <IotWebConf.h>                           // Library to take care of wifi connection & client settings
#include <ButtonDebounce.h>                       // Library to handle IR-sensor
#define FASTLED_ESP8266_D1_PIN_ORDER              // Needed for nodeMCU pin numbering
#include <FastLED.h>                              // Library for LEDs to signal
#include "mqttBlockSig.h"                         // Some of the client settings

CRGB leds[NUM_LED_DRIVERS];                       // Define the array of leds

// Make objects for the PubSubClient
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Make object for IR-sensor
ButtonDebounce irSensor(SENSOR1_PIN, 1500);       // 1,5 sec delay

// ------------------------------------------------------------------------------------------------------------------------------
// Variables set on the configuration web page

// Access point configuration
const char thingName[]              = APNAME;     // Initial AP name, used as SSID of the own Access Point
const char wifiInitialApPassword[]  = APPASSWORD; // Initial password, used to the own Access Point

// Name of the server we want to get config from
const char cHost[]                  = CONFIG_HOST;
const int cPort                     = CONFIG_HOST_PORT;
const char cPath[]                  = CONFIG_HOST_FILE;
const char* configPath;
const char* tmpMqttServer;
String nodeConfig[2][4];

// Device configuration
char cfgMqttServer[STRING_LEN];
char cfgMqttPort[NUMBER_LEN];
char cfgMqttTopic[STRING_LEN];
char cfgDeviceId[STRING_LEN];
char cfgDeviceName[STRING_LEN];
char cfgSignal1Type[STRING_LEN];
char cfgSignal2Type[STRING_LEN];
char cfgLedBrightness[NUMBER_LEN];
char cfgSignalm1Id[STRING_LEN];
char cfgSignalm1Dist[NUMBER_LEN];
char cfgSignalm2Id[STRING_LEN];
char cfgSignalm2Dist[NUMBER_LEN];

// ------------------------------------------------------------------------------------------------------------------------------
// Define MQTT topic variables

// Variable for number of Signals and Blocks
const int nbrSigTopics = 2;
const int nbrBloTopics = 1;
String pubSigTopic[nbrSigTopics];
String pubBloTopic[nbrBloTopics];

// Variable for topics to subscribe to
const int nbrSubTopics = 2;
String subTopic[nbrSubTopics];

// Variable for topics to publish to
const int nbrPubTopics = 5;
String pubTopic[nbrPubTopics];
String pubTopicContent[nbrPubTopics];

// Often used topics
String pubDeviceStateTopic;                       // Topic showing the state of device
String pubDirTopic;                               // Traffic direction [upwards, downwards]
const byte NORETAIN = 0;                          // Used to publish topics as NOT retained
const byte RETAIN   = 1;                          // Used to publish topics as retained


// ------------------------------------------------------------------------------------------------------------------------------
// IotWebConfig variables

// Indicate if it is time to reset the client or connect to MQTT
bool needMqttConnect  = false;
//bool needReset        = false;

// Callback method declarations
void configSaved();
bool formValidator();

// Make objects for IotWebConf
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);


// ------------------------------------------------------------------------------------------------------------------------------
// Define settings to show up on configuration web page
// IotWebConfParameter(label, id, valueBuffer, length, type = "text", 
//                     placeholder = NULL, defaultValue = NULL,
//                     customHtml = NULL, visible = true)
    
// Add a new parameter section on the settings web page
IotWebConfSeparator separator1        = IotWebConfSeparator("MQTT-inställningar");

// Add configuration for MQTT
IotWebConfParameter webMqttServer     = IotWebConfParameter("MQTT IP-adress", "mqttServer", cfgMqttServer, STRING_LEN);
IotWebConfParameter webMqttPort       = IotWebConfParameter("MQTT-port", "mqttPort", cfgMqttPort, NUMBER_LEN, "number",
                                                            "1883");
IotWebConfParameter webMqttTopic      = IotWebConfParameter("MQTT root topic", "mqttTopic", cfgMqttTopic, STRING_LEN, "text",
                                                             ROOTTOPIC);
// Add a new parameter section on the settings web page
IotWebConfSeparator separator2        = IotWebConfSeparator("Enhetens inställningar");

// Add configuration for Device
IotWebConfParameter webDeviceId       = IotWebConfParameter("Modulens unika id", "deviceId", cfgDeviceId, STRING_LEN);
IotWebConfParameter webDeviceName     = IotWebConfParameter("Modulens namn", "deviceName", cfgDeviceName, STRING_LEN);

// Add configuration for signals
IotWebConfSeparator separator3        = IotWebConfSeparator("Egna Signaler");
IotWebConfParameter webSignal1Type    = IotWebConfParameter("Signaltyp nedspår", "signal1Type", cfgSignal1Type, STRING_LEN, "text",
                                                            "ex. Hsi5", SIG_ONE_TYPE);
IotWebConfParameter webSignal2Type    = IotWebConfParameter("Signaltyp uppspår", "signal2Type", cfgSignal2Type, STRING_LEN, "text",
                                                            "ex. Hsi5", SIG_TWO_TYPE);
IotWebConfParameter webLedBrightness  = IotWebConfParameter("LED Ljusstyrka", "LedBrightness", cfgLedBrightness, NUMBER_LEN, "number",
                                                            "1..255");
// Add configuration for surrounding modules
IotWebConfSeparator separator4        = IotWebConfSeparator("Signalmodul nedspår");
IotWebConfParameter webSignalm1Id     = IotWebConfParameter("Signalmodulid", "signalm1Id", cfgSignalm1Id, STRING_LEN);
IotWebConfParameter webSignalm1Dist   = IotWebConfParameter("Avstånd i m", "signalm1Dist", cfgSignalm1Dist, STRING_LEN, "text", "ex. 3.0");
IotWebConfSeparator separator5        = IotWebConfSeparator("Signalmodul uppspår");
IotWebConfParameter webSignalm2Id     = IotWebConfParameter("Signalmodulid", "signalm2Id", cfgSignalm2Id, STRING_LEN);
IotWebConfParameter webSignalm2Dist   = IotWebConfParameter("Avstånd i m", "signalm2Dist", cfgSignalm2Dist, STRING_LEN, "text", "ex. 2.5");

// ------------------------------------------------------------------------------------------------------------------------------
// Other variables

// Variables to be set after getting configuration from file
String deviceID;
String deviceName;
String moduleName[2];
String moduleDistance[2];
String signalId[2];
String mqttServer;
String mqttRootTopic;
int signalType[2];
int ledBrightness;
int mqttPort;

// Variables for Signalaspects, Block state and Train direction state used in topics
int LEDdrive1;
int LEDdrive2;
const int nbrSigAspects           = 7;
const int nbrSigTypes             = 7;
const int nbrBloStates            = 2;
const int nbrDirStates            = 3;
String sigAspects[nbrSigAspects]  = {TOP_SIG_STOP, TOP_SIG_CLEAR, TOP_SIG_CAUTION, TOP_SIG_CLEAR_W_STOP,
                                     TOP_SIG_CLEAR_W_CLEAR, TOP_SIG_CLEAR_W_CAUTION, TOP_SIG_SHORT_ROUTE};
String bloStates[nbrBloStates]    = {TOP_BLO_OCCU, TOP_BLO_FREE};
String dirState[nbrDirStates]     = {TOP_DIR_FWRD, TOP_DIR_BCKW};
String signalTypes[nbrSigTypes]   = {SIG_NOTUSED, SIG_HSI2, SIG_HSI3, SIG_HSI4, SIG_HSI5, SIG_FSI2, SIG_FSI3};

// Variables to store actual states
int traffDir                      = DIR_FWRD;                 // Traffic direction
int ownSigAspect[2]               = {SIG_STOP, SIG_STOP};     // Own signal forward and backward current aspect
int oldSigAspect[2]               = {SIG_STOP, SIG_STOP};     // Own signal forward and backward old aspect
int tmpSigAspect[2];                                          // Own signal forward and backward temp aspect
int ownBloState[2]                = {BLO_FREE, BLO_FREE};     // forward line and backward line block state
int modSigAspect[2][2]            = {{SIG_STOP, SIG_STOP},    // Module down signal forward and backward aspects
                                     {SIG_STOP, SIG_STOP}};   // Module up signal forward and backward aspects
int modBloState[2]                = {BLO_FREE,                // Module up forward block state
                                     BLO_FREE};               // Module down forward block state
int sig;                                                      // Current signal in progress

// Variables to indicate if action is in progress
int sensorState     = BLO_FREE;                               // To get two states from the IR-sensor
bool sigFlash[2]    = {false, true};                          // [0] Flash ongoing, [1] Flash done
bool sigFading[2]   = {false, false};                         // [0] oldsigAspect, [1] newsigAspect
int flashTimes      = FLASH_AT_STARTUP;
unsigned long currTime;
unsigned long flashTime;

// Debug
bool debug          = SET_DEBUG;                              // Set to true for debug messages in Serial monitor (9600 baud)
String dbText       = "Main   : ";                            // Debug text. Occurs first in every debug output

//HttpClient http = HttpClient(wifiClient, cHost, cPort);
HttpClient http(wifiClient, cHost, cPort);

// ------------------------------------------------------------------------------------------------------------------------------
//  Standard setup function
// ------------------------------------------------------------------------------------------------------------------------------
void setup() {

  // Setup Arduino IDE serial monitor for "debugging"
  if (debug) {Serial.begin(19200);Serial.println("");}
  if (debug) {Serial.println(dbText+"Starting setup");}

  // ----------------------------------------------------------------------------------------------------------------------------
  // Set-up LED drivers
  FastLED.addLeds<WS2811, SIG_DATA_PIN, RGB>(leds, NUM_LED_DRIVERS);

  // ----------------------------------------------------------------------------------------------------------------------------
  // IR-sensor start
  if (debug) {Serial.println(dbText+"IR-sensor setup");}
  if (debug) {Serial.println(dbText+"Sensor pin = "+SENSOR1_PIN);}

  // Add the callback function to be called when the IR-sensor is changed
  irSensor.setCallback(sensorChange);

  // ----------------------------------------------------------------------------------------------------------------------------
  // IotWebConfig start
  if (debug) {Serial.println(dbText+"IotWebConf setup");}

  // Adding up items to show on config web page
  iotWebConf.addParameter(&separator1);                       // MQTT-inställningar
  iotWebConf.addParameter(&webMqttServer);                    // MQTT IP-adress
  iotWebConf.addParameter(&webMqttPort);                      // MQTT-port
  iotWebConf.addParameter(&webMqttTopic);                     // MQTT root topic
  iotWebConf.addParameter(&separator2);                       // Enhetens inställningar
  iotWebConf.addParameter(&webDeviceId);                      // Modulens unika id
  iotWebConf.addParameter(&webDeviceName);                    // Modulens namn
  iotWebConf.addParameter(&separator3);                       // Signaler
  iotWebConf.addParameter(&webSignal1Type);                   // Signaltyp nedspår
  iotWebConf.addParameter(&webSignal2Type);                   // Signaltyp uppspår
  iotWebConf.addParameter(&webLedBrightness);                 // LED ljusstyrka
  iotWebConf.addParameter(&separator4);                       // Signalmodul nedspår
  iotWebConf.addParameter(&webSignalm1Id);                    // Signalmodulid
  iotWebConf.addParameter(&webSignalm1Dist);                  // Avstånd i m
  iotWebConf.addParameter(&separator5);                       // Signalmodul uppspår
  iotWebConf.addParameter(&webSignalm2Id);                    // Signalmodulid
  iotWebConf.addParameter(&webSignalm2Dist);                  // Avstånd i m

  // Validate data input
  iotWebConf.setFormValidator(&formValidator);

  // Show/set AP timeout on web page
// iotWebConf.getApTimeoutParameter()->visible = true;

   iotWebConf.setStatusPin(STATUS_PIN);
   if (debug) {Serial.println(dbText+"Status pin = "+STATUS_PIN);}

// iotWebConf.setConfigPin(CONFIG_PIN);
// if (debug) {Serial.println(dbText+"Config pin = "+CONFIG_PIN);}

  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setWifiConnectionCallback(&wifiConnected);

  // Setting default configuration
  bool validConfig = iotWebConf.init();
  if (!validConfig) {
    // Set configuration default values
    mqttRootTopic           = ROOTTOPIC;
    deviceName              = APNAME;
    String tmpNo            = String(random(2147483647));
    deviceID                = deviceName+"-"+tmpNo;
    for (int i = 0; i< nbrSigTypes; i++) {
      if (signalTypes[i] == SIG_ONE_TYPE) {signalType[SIGNAL_FWRD] = i;}
      if (signalTypes[i] == SIG_TWO_TYPE) {signalType[SIGNAL_BCKW] = i;}
    }
    ledBrightness           = LED_BRIGHTNESS;
  }

  else {
    deviceID                = String(cfgDeviceId);
    deviceID.toLowerCase();
    deviceName              = String(cfgDeviceName);
    mqttPort                = atoi(cfgMqttPort);
    mqttServer              = String(cfgMqttServer);
    moduleName[MODULE_BCKW] = String(cfgSignalm1Id);
    moduleName[MODULE_FWRD] = String(cfgSignalm2Id);
    mqttRootTopic           = String(cfgMqttTopic);
    for (int i = 0; i< nbrSigTypes; i++) {
      if (signalTypes[i] == String(cfgSignal1Type)) {signalType[SIGNAL_BCKW] = i;}
      if (signalTypes[i] == String(cfgSignal2Type)) {signalType[SIGNAL_FWRD] = i;}
    }
    ledBrightness           = atoi(cfgLedBrightness);
  }

  FastLED.setBrightness(ledBrightness);

  // Set up required URL handlers for the config web pages
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  delay(2000);                                                // Wait for IotWebServer to start network connection

  // ----------------------------------------------------------------------------------------------------------------------------
  // Flash all LED at start-up
  if (flashTimes > 0) {updateSignals(START_UP);}

  currTime = millis();
}


// ------------------------------------------------------------------------------------------------------------------------------
// (Re)connects to MQTT broker and subscribes to one or more topics
// ------------------------------------------------------------------------------------------------------------------------------
bool mqttConnect() {

  char tmpTopic[254];
  char tmpContent[254];
  char tmpID[deviceID.length()];                                                        // For converting deviceID
  char* tmpMessage = "lost";                                                            // Status message in Last Will
  
  // Set up broker and assemble topics to subscribe to and publish
  setupBroker();
  setupTopics();

  // Convert String to char* for last will message
  deviceID.toCharArray(tmpID, deviceID.length()+1);
  pubDeviceStateTopic.toCharArray(tmpTopic, pubDeviceStateTopic.length()+1);
  
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    if (debug) {Serial.print(dbText+"MQTT connection...");}

    // Attempt to connect
    // boolean connect (tmpID, pubDeviceStateTopic, willQoS, willRetain, willMessage)
    if (mqttClient.connect(tmpID, tmpTopic, 0, true, tmpMessage)) {

      if (debug) {Serial.println("connected");}
      if (debug) {Serial.print(dbText+"MQTT client id = ");}
      if (debug) {Serial.println(cfgDeviceId);}

      // Subscribe to all topics
      if (debug) {Serial.println(dbText+"Subscribing to:");}

      for (int i=0; i < nbrSubTopics; i++){
        // Convert String to char* for the mqttClient.subribe() function to work
        subTopic[i].toCharArray(tmpTopic, subTopic[i].length()+1);
  
        // ... print topic
        if (debug) {Serial.println(dbText+" - "+tmpTopic);}

        //   ... and subscribe to topic
        mqttClient.subscribe(tmpTopic);
      }

      // Publish to all topics
      if (debug) {Serial.println(dbText+"Publishing:");}

      for (int i=0; i < nbrPubTopics; i++){
        // Convert String to char* for the mqttClient.publish() function to work
        pubTopic[i].toCharArray(tmpTopic, pubTopic[i].length()+1);
        pubTopicContent[i].toCharArray(tmpContent, pubTopicContent[i].length()+1);

        // ... print topic
        if (debug) {Serial.print(dbText+" - "+tmpTopic);}
        if (debug) {Serial.print(" = ");}
        if (debug) {Serial.println(tmpContent);}

        // ... and subscribe to topic
        mqttClient.publish(tmpTopic, tmpContent, true);
      }
    }

    else {
      // Show why the connection failed
      if (debug) {Serial.print(dbText+"failed, rc=");}
      if (debug) {Serial.print(mqttClient.state());}
      if (debug) {Serial.println(", try again in 5 seconds");}

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  // Set device status to "ready"
  updateSignals(INITIAL_SET);
  
  mqttPublish(pubSigTopic[SIGNAL_FWRD], sigAspects[ownSigAspect[SIGNAL_FWRD]], RETAIN); // Own current forward signal aspects
  mqttPublish(pubSigTopic[SIGNAL_BCKW], sigAspects[ownSigAspect[SIGNAL_BCKW]], RETAIN); // Own current backward signal aspects
  mqttPublish(pubBloTopic[BLO_ONE], bloStates[sensorState], RETAIN);                    // Own current block state
  mqttPublish(pubDirTopic, dirState[traffDir], RETAIN);                                 // Current traffic direction
  mqttPublish(pubDeviceStateTopic, "ready", RETAIN);                                    // Node ready
  return true;
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function to handle MQTT messages sent to this device
// ------------------------------------------------------------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  // Don't know why this have to be done :-(
  payload[length] = '\0';

  // Make strings
  String msg = String((char*)payload);
  String tpc = String((char*)topic);

  // Print the topic and payload
  if (debug) {Serial.println(dbText+" [424] - Recieved: "+tpc+" = "+msg);}

  if (tpc == mqttRootTopic+"/"+moduleName[MODULE_FWRD]+"/signal/up/"+SIG_ONE_ID+"/state") {       // Forward signal on single track forward
    handleSignal(moduleName[MODULE_FWRD], SIGNAL_FWRD, msg);
  }

  else if (tpc == mqttRootTopic+"/"+moduleName[MODULE_FWRD]+"/signal/up/"+SIG_TWO_ID+"/state") {  // Backward signal on single track forward
    handleSignal(moduleName[MODULE_FWRD], SIGNAL_BCKW, msg);
  }

  else if (tpc == mqttRootTopic+"/"+moduleName[MODULE_BCKW]+"/signal/up/"+SIG_ONE_ID+"/state") {  // Forward signal on single track backwards
    handleSignal(moduleName[MODULE_BCKW], SIGNAL_FWRD, msg);
  }

  else if (tpc == mqttRootTopic+"/"+moduleName[MODULE_BCKW]+"/signal/up/"+SIG_TWO_ID+"/state") {  // Backward signal on single track backwards
    handleSignal(moduleName[MODULE_BCKW], SIGNAL_BCKW, msg);
  }

  else if (tpc == mqttRootTopic+"/"+moduleName[MODULE_FWRD]+"/block/up/"+BLO_ONE_ID+"/state") {   // Block on module forward
    handleBlock(moduleName[MODULE_FWRD], BLO_ONE, msg);
  }

  else if (tpc == mqttRootTopic+"/"+moduleName[MODULE_BCKW]+"/block/up/"+BLO_ONE_ID+"/state") {   // Block on module backwards
    handleBlock(moduleName[MODULE_BCKW], BLO_ONE, msg);
  }

  else if (tpc == mqttRootTopic+"/"+moduleName[MODULE_FWRD]+"/traffic/up/"+TOP_DIR+"/state") {    // Traffic direction on single track
    handleDirection(moduleName[MODULE_FWRD], TOP_DIR, msg);
  }

  else if (tpc == mqttRootTopic+"/"+moduleName[MODULE_BCKW]+"/traffic/up/"+TOP_DIR+"/state") {    // Traffic direction on single track
    handleDirection(moduleName[MODULE_BCKW], TOP_DIR, msg);
  }
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Publish message to MQTT broker
// ------------------------------------------------------------------------------------------------------------------------------
void mqttPublish(String pbTopic, String pbPayload, byte retain) {

  // Convert String to char* for the mqttClient.publish() function to work
  char msg[pbPayload.length()+1];
  pbPayload.toCharArray(msg, pbPayload.length()+1);
  char tpc[pbTopic.length()+1];
  pbTopic.toCharArray(tpc, pbTopic.length()+1);

  // Report back to pubTopic[]
  int check = mqttClient.publish(tpc, msg, retain);

  // TODO check "check" integer to see if all went ok

  // Print information
  if (debug) {Serial.println(dbText+" [477] - Sending: "+pbTopic+" = "+pbPayload);}
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function that gets called when the IR-sensor is changing
// ------------------------------------------------------------------------------------------------------------------------------
void sensorChange(const int state) {

  if (state != sensorState) {

    switch (state) {
      case BLO_OCCU:
        if (traffDir == DIR_FWRD) {
          if (ownSigAspect[SIGNAL_FWRD] != SIG_STOP) {
            ownSigAspect[SIGNAL_FWRD] = SIG_STOP;                                                   // Set signal forward to stop
            if (debug) {Serial.println(dbText+" [493] - ownSigAspect[SIGNAL_FWRD] set "+sigAspects[ownSigAspect[SIGNAL_FWRD]]);}
            if (debug) {Serial.println(dbText+" [494] - Publish");}
            mqttPublish(pubSigTopic[SIGNAL_FWRD], sigAspects[ownSigAspect[SIGNAL_FWRD]], RETAIN);
          }
          ownBloState[LINE_FWRD] = state;                                                           // Set forward line to occupied
          if (debug) {Serial.println(dbText+" [498] - Line -> "+bloStates[state]);}
        }

        else {
          if (ownSigAspect[SIGNAL_BCKW] != SIG_STOP) {
            ownSigAspect[SIGNAL_BCKW] = SIG_STOP;                                                   // Set signal backward to stop
            if (debug) {Serial.println(dbText+" [504] - ownSigAspect[SIGNAL_BCKW] set "+sigAspects[ownSigAspect[SIGNAL_BCKW]]);}
            if (debug) {Serial.println(dbText+" [505] - Publish");}
            mqttPublish(pubSigTopic[SIGNAL_BCKW], sigAspects[ownSigAspect[SIGNAL_BCKW]], RETAIN);
          }
          ownBloState[LINE_BCKW] = state;                                                           // Set backward line to occupied
          if (debug) {Serial.println(dbText+" [509] - Line <- "+bloStates[state]);}
        }
      break;

      case BLO_FREE:
        if (traffDir == DIR_FWRD) {
          ownBloState[LINE_BCKW] = state;                                                           // Set backward line to free
          if (debug) {Serial.println(dbText+" [516] - Line <- "+bloStates[state]);}
        }

        else {
          ownBloState[LINE_FWRD] = state;                                                           // Set forward line to free
          if (debug) {Serial.println(dbText+" [521] - Line -> "+bloStates[state]);}
        }
      break;
    }

    if (debug) {Serial.println(dbText+" [526] - IR-sensor indicating "+bloStates[state]);}

    // Publish own block state
    if (debug) {Serial.println(dbText+" [529] - Publish");}
    mqttPublish(pubBloTopic[BLO_ONE], bloStates[state], RETAIN);  

    sensorState = state;
  }
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function that gets called when a block message is received
// ------------------------------------------------------------------------------------------------------------------------------
void handleBlock(String module, int block, String msg) {

  int blstate = 255;

  for (int i = 0; i < nbrBloStates; i++) {
    if (msg == bloStates[i]) {
      blstate = i;
    }
  }

  switch (blstate) {
    case BLO_OCCU:
    case BLO_FREE:
      if (module == moduleName[MODULE_FWRD]) {                                                      // Module forward track
        if (block == BLO_ONE) {
          if (traffDir == DIR_BCKW && blstate == BLO_OCCU) {
            ownBloState[LINE_FWRD] = blstate;                                                       // Set forward line to occupied
            if (debug) {Serial.println(dbText+" [557] - Line -> "+bloStates[blstate]);}
          }

          else if (traffDir == DIR_FWRD && blstate == BLO_FREE) {
            ownBloState[LINE_FWRD] = blstate;                                                       // Set forward line to free
            if (debug) {Serial.println(dbText+" [562] - Line -> "+bloStates[blstate]);}
          }

          modBloState[MODULE_FWRD] = blstate;                                                       // Set forward module state
          if (debug) {Serial.println(dbText+" [566] - Module -> "+bloStates[blstate]);}
        }
      }

      else if (module == moduleName[MODULE_BCKW]) {                                                 // Module backwards track
        if (block == BLO_ONE) {
          if (traffDir == DIR_FWRD && blstate == BLO_OCCU) {
            ownBloState[LINE_BCKW] = blstate;                                                       // Set backward line to occupied
            if (debug) {Serial.println(dbText+" [574] - Line <- "+bloStates[blstate]);}
          }

          else if (traffDir == DIR_BCKW && blstate == BLO_FREE) {
            ownBloState[LINE_BCKW] = blstate;                                                       // Set backward line to free
            if (debug) {Serial.println(dbText+" [579] - Line <- "+bloStates[blstate]);}
          }

          modBloState[MODULE_BCKW] = blstate;                                                       // Set backward module state
          if (debug) {Serial.println(dbText+" [583] - Module <- "+bloStates[blstate]);}
        }
      }
    break;

    default:
      if (debug) {Serial.println(dbText+" [589] - Message '"+msg+"' not supported");}
    break;
  }
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function that gets called when a direction message is received
// ------------------------------------------------------------------------------------------------------------------------------
void handleDirection(String module, String dir, String msg) {

  int direction = 255;

  for (int i = 0; i < nbrDirStates; i++) {
    if (msg == dirState[i]) {
      direction = i;
    }
  }

  switch (direction) {
    case DIR_FWRD:
      if (ownSigAspect[SIGNAL_FWRD] == SIG_STOP &&
          ownSigAspect[SIGNAL_BCKW] == SIG_STOP &&
          module == moduleName[MODULE_BCKW]) {
        traffDir = direction;
      }
      // Publish new direction
      if (debug) {Serial.println(dbText+" [616] - Publish");}
      mqttPublish(pubDirTopic, dirState[traffDir], RETAIN);

    case DIR_BCKW:
      if (ownSigAspect[SIGNAL_FWRD] == SIG_STOP &&
          ownSigAspect[SIGNAL_BCKW] == SIG_STOP &&
          module == moduleName[MODULE_FWRD]) {
        traffDir = direction;
      }
      // Publish new direction
      if (debug) {Serial.println(dbText+" [626] - Publish");}
      mqttPublish(pubDirTopic, dirState[traffDir], RETAIN);

      if (debug) {
        switch (traffDir) {
          case DIR_FWRD:
            Serial.println(dbText+" [632] - Traffic "+dir+" ->");
          break;

          case DIR_BCKW:
            Serial.println(dbText+" [636] - Traffic "+dir+" <-");
          break;
        }
      }
    break;

    default:
      if (debug) {Serial.println(dbText+" [643] - Message '"+msg+"' not supported");}
    break;
  }
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function that gets called when a signal message is received
// ------------------------------------------------------------------------------------------------------------------------------
void handleSignal(String module, int signal, String msg) {

  int aspect = 255;

  for (int i = 0; i < nbrSigAspects; i++) {
    if (msg == sigAspects[i]) {
      aspect = i;
      oldSigAspect[signal] = ownSigAspect[signal];
      if (debug) {Serial.println(dbText+" [660] - oldSigAspect[sig] set "+sigAspects[oldSigAspect[signal]]);}
    }
  }
  sig = signal;

  switch (aspect) {
    case SIG_CLEAR:                                                                     // CLEAR received
    case SIG_CLEAR_W_CLEAR:
    case SIG_CLEAR_W_CAUTION:
    case SIG_CLEAR_W_STOP:
      if (module == moduleName[MODULE_FWRD]) {                                          // Module forward track
        modSigAspect[MODULE_FWRD][sig] = aspect;

        switch (sig) {
          case SIGNAL_FWRD:                                                             // Signal forward track clear
            if (traffDir == DIR_FWRD && modSigAspect[MODULE_BCKW][sig] != SIG_STOP) {

              switch (signalType[sig]) {
                case HSI2:
                case HSI3:
                  ownSigAspect[sig] = SIG_CLEAR;
                  if (debug) {Serial.println(dbText+" [681] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI4:
                case HSI5:
                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_CLEAR;
                  if (debug) {Serial.println(dbText+" [689] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_BCKW) {
              ownSigAspect[sig] = SIG_STOP;
              if (debug) {Serial.println(dbText+" [696] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
            }
          break;

          case SIGNAL_BCKW:                                                             // Signal backwards track clear
            if (traffDir == DIR_BCKW && modSigAspect[MODULE_BCKW][sig] == SIG_STOP) {

              switch (signalType[sig]) {
                case HSI2:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+" [706] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI3:
                  ownSigAspect[sig] = SIG_CAUTION;
                  if (debug) {Serial.println(dbText+" [711] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI4:
                case HSI5:
                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_STOP;
                  if (debug) {Serial.println(dbText+" [719] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_BCKW &&
                    (modSigAspect[MODULE_BCKW][sig] == SIG_CAUTION ||
                     modSigAspect[MODULE_BCKW][sig] == SIG_SHORT_ROUTE)) {

              switch (signalType[sig]) { 
                case HSI2:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+" [731] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI3:
                  ownSigAspect[sig] = SIG_CAUTION;
                  if (debug) {Serial.println(dbText+" [736] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI4:
                case HSI5:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_CAUTION;
                  if (debug) {Serial.println(dbText+" [743] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case FSI2:
                  ownSigAspect[sig] = SIG_CLEAR_W_STOP;
                  if (debug) {Serial.println(dbText+" [748] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_BCKW &&
                    (modSigAspect[MODULE_BCKW][sig] == SIG_CLEAR ||
                     modSigAspect[MODULE_BCKW][sig] == SIG_CLEAR_W_CLEAR ||
                     modSigAspect[MODULE_BCKW][sig] == SIG_CLEAR_W_CAUTION ||
                     modSigAspect[MODULE_BCKW][sig] == SIG_CLEAR_W_STOP)) {

              switch (signalType[sig]) { 
                case HSI2:
                case HSI3:
                  ownSigAspect[sig] = SIG_CLEAR;
                  if (debug) {Serial.println(dbText+" [763] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI4:
                case HSI5:
                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_CLEAR;
                  if (debug) {Serial.println(dbText+" [771] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_FWRD) {
              ownSigAspect[sig] = SIG_STOP;
              if (debug) {Serial.println(dbText+" [778] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
            }
          break;
        }
      }

      else if (module == moduleName[MODULE_BCKW]) {                                     // Module backwards track
        modSigAspect[MODULE_BCKW][sig] = aspect;

        switch (sig) {
          case SIGNAL_FWRD:                                                             // Signal forward track clear
            if (traffDir == DIR_FWRD &&
                modSigAspect[MODULE_FWRD][sig] == SIG_STOP) {

              switch (signalType[sig]) {
                case HSI2:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+" [795] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI3:
                  ownSigAspect[sig] = SIG_CAUTION;
                  if (debug) {Serial.println(dbText+" [800] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI4:
                case HSI5:
                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_STOP;
                  if (debug) {Serial.println(dbText+" [808] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_FWRD &&
                    (modSigAspect[MODULE_FWRD][sig] == SIG_CAUTION ||
                     modSigAspect[MODULE_FWRD][sig] == SIG_SHORT_ROUTE)) {

              switch (signalType[sig]) { 
                case HSI2:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+" [820] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI3:
                  ownSigAspect[sig] = SIG_CAUTION;
                  if (debug) {Serial.println(dbText+" [825] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI4:
                case HSI5:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_CAUTION;
                  if (debug) {Serial.println(dbText+" [832] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case FSI2:
                  ownSigAspect[sig] = SIG_CLEAR_W_STOP;
                  if (debug) {Serial.println(dbText+" [837] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_FWRD &&
                    (modSigAspect[MODULE_FWRD][sig] == SIG_CLEAR ||
                     modSigAspect[MODULE_FWRD][sig] == SIG_CLEAR_W_CLEAR ||
                     modSigAspect[MODULE_FWRD][sig] == SIG_CLEAR_W_CAUTION ||
                     modSigAspect[MODULE_FWRD][sig] == SIG_CLEAR_W_STOP)) {

              switch (signalType[sig]) { 
                case HSI2:
                case HSI3:
                  ownSigAspect[sig] = SIG_CLEAR;
                  if (debug) {Serial.println(dbText+" [852] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI4:
                case HSI5:
                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_CLEAR;
                  if (debug) {Serial.println(dbText+" [860] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_BCKW) {
              ownSigAspect[sig] = SIG_STOP;
              if (debug) {Serial.println(dbText+" [867] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
            }
          break;

          case SIGNAL_BCKW:                                                             // Signal backwards track clear
            if (traffDir == DIR_BCKW && modSigAspect[MODULE_FWRD][sig] != SIG_STOP) {

              switch (signalType[sig]) {
                case HSI2:
                case HSI3:
                  ownSigAspect[sig] = SIG_CLEAR;
                  if (debug) {Serial.println(dbText+" [878] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI4:
                case HSI5:
                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_CLEAR;
                  if (debug) {Serial.println(dbText+" [886] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_FWRD) {
              ownSigAspect[sig] = SIG_STOP;
              if (debug) {Serial.println(dbText+" [893] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
            }
          break;
        }
      }
    break;

    case SIG_CAUTION:                                                                   // CAUTION received
    case SIG_SHORT_ROUTE:
      if (module == moduleName[MODULE_FWRD]) {                                          // Module forward track
        modSigAspect[MODULE_FWRD][sig] = aspect;

        switch (sig) { 
          case SIGNAL_FWRD:                                                             // Signal forward track caution
            if (traffDir == DIR_FWRD && modSigAspect[MODULE_BCKW][sig] != SIG_STOP) {

              switch (signalType[sig]) {
                case HSI2:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+" [912] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI3:
                case HSI4:
                  ownSigAspect[sig] = SIG_CAUTION;
                  if (debug) {Serial.println(dbText+" [918] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI5:
                  ownSigAspect[sig] = SIG_CLEAR_W_CAUTION;
                  if (debug) {Serial.println(dbText+" [923] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_CLEAR;
                  if (debug) {Serial.println(dbText+" [929] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_FWRD && modSigAspect[MODULE_BCKW][sig] == SIG_STOP && 
                     ownSigAspect[sig] != SIG_STOP) {

              switch (signalType[sig]) {
                case HSI2:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+" [940] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI3:
                case HSI4:
                  ownSigAspect[sig] = SIG_CAUTION;
                  if (debug) {Serial.println(dbText+" [946] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI5:
                  ownSigAspect[sig] = SIG_CLEAR_W_CAUTION;
                  if (debug) {Serial.println(dbText+" [951] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_CLEAR;
                  if (debug) {Serial.println(dbText+" [957] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_BCKW) {
              ownSigAspect[sig] = SIG_STOP;
              if (debug) {Serial.println(dbText+" [964] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
            }
          break;

          case SIGNAL_BCKW:                                                             // Signal backwards track caution
            // no action
          break;
        }
      }

      else if (module == moduleName[MODULE_BCKW]) {                                     // Module backwards track
        modSigAspect[MODULE_BCKW][sig] = aspect;

        switch (sig) {
          case SIGNAL_FWRD:                                                             // Signal forward track caution
            // no action
          break;

          case SIGNAL_BCKW:                                                             // Signal backwards track caution
            if (traffDir == DIR_BCKW && modSigAspect[MODULE_BCKW][sig] != SIG_STOP) {

              switch (signalType[sig]) {
                case HSI2:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+" [988] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI3:
                case HSI4:
                  ownSigAspect[sig] = SIG_CAUTION;
                  if (debug) {Serial.println(dbText+" [994] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI5:
                  ownSigAspect[sig] = SIG_CLEAR_W_CAUTION;
                  if (debug) {Serial.println(dbText+" [999] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_CLEAR;
                  if (debug) {Serial.println(dbText+"[1005] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_FWRD) {
              ownSigAspect[sig] = SIG_STOP;
              if (debug) {Serial.println(dbText+"[1012] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
            }
          break;
        }
      }
    break;

    case SIG_STOP:                                                                      // STOP received

      if (module == moduleName[MODULE_FWRD]) {                                          // Module forward track
        modSigAspect[MODULE_FWRD][sig] = aspect;

        switch (sig) {
          case SIGNAL_FWRD:                                                             // Signal forward track stop
            if (traffDir == DIR_FWRD && modSigAspect[MODULE_BCKW][sig] != SIG_STOP) {

              switch (signalType[sig]) {
                case HSI2:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+"[1031] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI3:
                  ownSigAspect[sig] = SIG_CAUTION;
                  if (debug) {Serial.println(dbText+"[1036] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case HSI4:
                case HSI5:
                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_STOP;
                  if (debug) {Serial.println(dbText+"[1044] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_FWRD && modSigAspect[MODULE_BCKW][sig] == SIG_STOP && 
                     ownSigAspect[sig] != SIG_STOP) {

              switch (signalType[sig]) {
                case HSI2:
                case HSI3:
                case HSI4:
                case HSI5:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+"[1058] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_STOP;
                  if (debug) {Serial.println(dbText+"[1064] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }

            else if (traffDir == DIR_BCKW) {
              ownSigAspect[sig] = SIG_STOP;
              if (debug) {Serial.println(dbText+"[1071] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
            }
          break;

          case SIGNAL_BCKW:                                                             // Signal backwards track stop
           if (traffDir == DIR_BCKW && modBloState[MODULE_FWRD] == BLO_FREE &&
               ownSigAspect[sig] != SIG_STOP) {

             switch (signalType[sig]) {
                case HSI2:
                case HSI3:
                case HSI4:
                case HSI5:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+"[1085] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_STOP;
                  if (debug) {Serial.println(dbText+"[1091] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }
          break;
        }
      }

      else if (module == moduleName[MODULE_BCKW]) {                                     // Module backwards track
        modSigAspect[MODULE_BCKW][sig] = aspect;

        switch (sig) {
          case SIGNAL_FWRD:                                                             // Signal forward track stop
           if (traffDir == DIR_FWRD && modBloState[MODULE_BCKW] == BLO_FREE &&
               ownSigAspect[sig] != SIG_STOP) {

             switch (signalType[sig]) {
                case HSI2:
                case HSI3:
                case HSI4:
                case HSI5:
                  ownSigAspect[sig] = SIG_STOP;
                  if (debug) {Serial.println(dbText+"[1113] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;

                case FSI2:
                case FSI3:
                  ownSigAspect[sig] = SIG_CLEAR_W_STOP;
                  if (debug) {Serial.println(dbText+"[1119] - ownSigAspect[sig] set "+sigAspects[ownSigAspect[sig]]);}
                break;
              }
            }
          break;

          case SIGNAL_BCKW:                                                             // Signal backwards track stop
            // no action
          break;
        }
      }
    break;

    default:
      if (debug) {Serial.println(dbText+"[1133] - Message '"+msg+"' not supported");}
    break;
  }

  if (signalType[sig] != SIG_NOTDEF && ownSigAspect[sig] != oldSigAspect[sig]) {

    if (debug) {Serial.println(dbText+"[1139] - Publish");}
    mqttPublish(pubSigTopic[sig], sigAspects[ownSigAspect[sig]], RETAIN);
  }
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Send Signal aspect out to the signals
// ------------------------------------------------------------------------------------------------------------------------------
void updateSignals(int job) {

  // job = START_UP     Start-up
  // job = INITIAL_SET  Set signal to Stop
  // job = RUNNING      Normal running

  if (ownSigAspect[SIGNAL_FWRD] != oldSigAspect[SIGNAL_FWRD] && sig == SIGNAL_FWRD) {
    LEDdrive1 = 0;                                                                      // Signal forward LEDdrive 0
    LEDdrive2 = 1;                                                                      // Signal forward LEDdrive 1
    if (debug) {Serial.println(dbText+"[1157] - Signal Forward");}
  }

  else if (ownSigAspect[SIGNAL_BCKW] != oldSigAspect[SIGNAL_BCKW] && sig == SIGNAL_BCKW) {
    LEDdrive1 = 2;                                                                      // Signal backward LEDdrive 2
    LEDdrive2 = 3;                                                                      // Signal backward LEDdrive 3
    if (debug) {Serial.println(dbText+"[1163] - Signal Backward");}
  }

  if ((signalType[sig] == FSI2 || signalType[sig] == FSI3) && sigFlash[FLASH_DONE]) {tmpSigAspect[sig] = ownSigAspect[sig];}

  switch (job) {
    case RUNNING:                                                                       // Normal job
      if ((signalType[sig] == FSI2 || signalType[sig] == FSI3) && 
            (millis() - flashTime) >= FLASH_FREQ2) {                                    // Distance signal

        switch (tmpSigAspect[sig]) { 
          case SIG_CLEAR_W_STOP:
            if (sigFlash[FLASH_DONE]) {
              leds[LEDdrive1][2] += UPDATE_STEP;
              if (leds[LEDdrive1][2] > 217) {
                leds[LEDdrive1][2] = ON;
                sigFlash[FLASH_DONE] = false;                                           // Flashing lite not reached dark
                if (debug) {Serial.println(dbText+"[1180] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                flashTime = millis();
              }
            }

            else {
              leds[LEDdrive1][2] -= UPDATE_STEP;
              if (leds[LEDdrive1][2] < 38) {
                leds[LEDdrive1][2] = OFF;
                sigFlash[FLASH_DONE] = true;                                            // Flashing lite reached dark
                if (debug) {Serial.println(dbText+"[1190] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                flashTime = millis();
                delay(20);
              }
            }
          break;

          case SIG_CLEAR_W_CLEAR:
            if (sigFlash[FLASH_DONE]) {
              leds[LEDdrive2][0] += UPDATE_STEP;
              if (leds[LEDdrive2][0] > 217) {
                leds[LEDdrive2][0] = ON;
                sigFlash[FLASH_DONE] = false;                                           // Flashing lite not reached dark
                if (debug) {Serial.println(dbText+"[1203] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                flashTime = millis();
              }
            }

            else {
              leds[LEDdrive2][0] -= UPDATE_STEP;
              if (leds[LEDdrive2][0] < 38) {
                leds[LEDdrive2][0] = OFF;
                sigFlash[FLASH_DONE] = true;                                            // Flashing lite reached dark
                if (debug) {Serial.println(dbText+"[1213] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                flashTime = millis();
                delay(20);
              }
            }
          break;

          case SIG_CLEAR_W_CAUTION:
            if (sigFlash[FLASH_DONE]) {
              leds[LEDdrive1][2] += UPDATE_STEP;
              leds[LEDdrive2][1] += UPDATE_STEP;
              if (leds[LEDdrive1][2] > 217) {
                leds[LEDdrive1][2] = ON;
                leds[LEDdrive2][1] = ON;
                sigFlash[FLASH_DONE] = false;                                         // Flashing lite not reached dark
                if (debug) {Serial.println(dbText+"[1228] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                flashTime = millis();
              }
            }

            else {
              leds[LEDdrive1][2] -= UPDATE_STEP;
              leds[LEDdrive2][1] -= UPDATE_STEP;
              if (leds[LEDdrive1][2] < 38) {
                leds[LEDdrive1][2] = OFF;
                leds[LEDdrive2][1] = OFF;
                sigFlash[FLASH_DONE] = true;                                          // Flashing lite reached dark
                if (debug) {Serial.println(dbText+"[1240] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                flashTime = millis();
              }
            }
          break;
        }
      }

      else if (ownSigAspect[sig] != oldSigAspect[sig] && sigFlash[FLASH_DONE]) {        // Hsi signal changed aspect

        if (!sigFading[OLD_ASPECT]) {                                                   // Fading down old aspect

          switch (oldSigAspect[sig]) {
            case SIG_STOP:
              leds[LEDdrive1][1] -= UPDATE_STEP;
              if (leds[LEDdrive1][1] < 38) {
                leds[LEDdrive1][1] = OFF;
                sigFading[OLD_ASPECT] = true;                                           // Fading down old aspect done
              }
            break;

            case SIG_CLEAR:
              leds[LEDdrive1][0] -= UPDATE_STEP;
              if (leds[LEDdrive1][0] < 38) {
                leds[LEDdrive1][0] = OFF;
                sigFading[OLD_ASPECT] = true;                                           // Fading down old aspect done
              }
            break;

            case SIG_CLEAR_W_STOP:
            case SIG_CLEAR_W_CLEAR:
            case SIG_CLEAR_W_CAUTION:
              if (ownSigAspect[sig] == SIG_STOP) {
                leds[LEDdrive1][0] -= UPDATE_STEP;
                if (leds[LEDdrive1][0] < 38) {
                  leds[LEDdrive1][0] = OFF;
                  sigFading[OLD_ASPECT] = true;                                         // Fading down old aspect done
                }
              }

              else {
                sigFading[OLD_ASPECT] = true;                                           // Fading down old aspect done
              }
            break;

            case SIG_CAUTION:
              leds[LEDdrive1][0] -= UPDATE_STEP;
              leds[LEDdrive1][2] -= UPDATE_STEP;
              if (leds[LEDdrive1][0] < 38) {
                leds[LEDdrive1][0] = OFF;
                leds[LEDdrive1][2] = OFF;
                sigFading[OLD_ASPECT] = true;                                           // Fading down old aspect done
              }
            break;
          }
        }

        else {                                                                          // Fading up new aspect

          switch (ownSigAspect[sig]) {
            case SIG_STOP:
              sigFlash[DO_FLASH] = false;                                               // Flashing aspect not needed
              if (debug) {Serial.println(dbText+"[1302] - sigFlash[DO_FLASH] set "+sigFlash[DO_FLASH]);}
              leds[LEDdrive1][1] += UPDATE_STEP;
              if (leds[LEDdrive1][1] > 217) {
                leds[LEDdrive1][1] = ON;
                sigFading[NEW_ASPECT] = true;                                           // Fading up new aspect done
              }
            break;

            case SIG_CLEAR:
              sigFlash[DO_FLASH] = false;                                               // Flashing aspect not needed
              if (debug) {Serial.println(dbText+"[1312] - sigFlash[DO_FLASH] set "+sigFlash[DO_FLASH]);}
              leds[LEDdrive1][0] += UPDATE_STEP;
              if (leds[LEDdrive1][0] > 217) {
                leds[LEDdrive1][0] = ON;
                sigFading[NEW_ASPECT] = true;                                           // Fading up new aspect done
              }
            break;

            case SIG_CLEAR_W_STOP:
            case SIG_CLEAR_W_CLEAR:
            case SIG_CLEAR_W_CAUTION:
              if (oldSigAspect[sig] == SIG_STOP) {
                sigFlash[DO_FLASH] = true;                                              // Flashing aspect needed
                if (debug) {Serial.println(dbText+"[1325] - sigFlash[DO_FLASH] set "+sigFlash[DO_FLASH]);}
                leds[LEDdrive1][0] += UPDATE_STEP;
                if (leds[LEDdrive1][0] > 217) {
                  leds[LEDdrive1][0] = ON;
                  sigFading[NEW_ASPECT] = true;                                         // Fading up new aspect done
                }
                tmpSigAspect[sig] = ownSigAspect[sig];
              }

              else {
                sigFlash[DO_FLASH] = true;                                              // Flashing aspect needed
                if (debug) {Serial.println(dbText+"[1336] - sigFlash[DO_FLASH] set "+sigFlash[DO_FLASH]);}
                sigFading[NEW_ASPECT] = true;                                           // Fading up new aspect done
                tmpSigAspect[sig] = ownSigAspect[sig];
              }
            break;

            case SIG_CAUTION:
              sigFlash[DO_FLASH] = false;                                               // Flashing aspect not needed
              if (debug) {Serial.println(dbText+"[1344] - sigFlash[DO_FLASH] set "+sigFlash[DO_FLASH]);}
              leds[LEDdrive1][0] += UPDATE_STEP;
              leds[LEDdrive1][2] += UPDATE_STEP;
              if (leds[LEDdrive1][0] > 217) {
                leds[LEDdrive1][0] = ON;
                leds[LEDdrive1][2] = ON;
                sigFading[NEW_ASPECT] = true;                                           // Fading up new aspect done
              }
            break;
          }
        }

        FastLED.show();
        if (sigFading[OLD_ASPECT] && sigFading[NEW_ASPECT]) {
          oldSigAspect[sig] = ownSigAspect[sig];
          sigFading[OLD_ASPECT] = false;                                                // Fading down old aspect not done
          sigFading[NEW_ASPECT] = false;                                                // Fading up new aspect not done
        }

        if (debug) {Serial.println(dbText+"[1363] - Signal updated");}
      }

      else if (sigFlash[DO_FLASH]) {                                                    // Flashing part of an Hsi aspect
        if ((millis() - flashTime) >= FLASH_FREQ1) {                                    // Distance signal combined with main signal

          switch (tmpSigAspect[sig]) { 
            case SIG_CLEAR_W_STOP:
              if (sigFlash[FLASH_DONE]) {
                leds[LEDdrive1][2] += UPDATE_STEP;
                if (leds[LEDdrive1][2] > 217) {
                  leds[LEDdrive1][2] = ON;
                  sigFlash[FLASH_DONE] = false;                                         // Flashing lite not reached dark
                  if (debug) {Serial.println(dbText+"[1376] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                  flashTime = millis();
                }
              }

              else {
                leds[LEDdrive1][2] -= UPDATE_STEP;
                if (leds[LEDdrive1][2] < 38) {
                  leds[LEDdrive1][2] = OFF;
                  sigFlash[FLASH_DONE] = true;                                          // Flashing lite reached dark
                  if (debug) {Serial.println(dbText+"[1386] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                  flashTime = millis();
                }
              }
            break;

            case SIG_CLEAR_W_CLEAR:
              if (sigFlash[FLASH_DONE]) {
                leds[LEDdrive2][0] += UPDATE_STEP;
                if (leds[LEDdrive2][0] > 217) {
                  leds[LEDdrive2][0] = ON;
                  sigFlash[FLASH_DONE] = false;                                         // Flashing lite not reached dark
                  if (debug) {Serial.println(dbText+"[1398] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                  flashTime = millis();
                }
              }

              else {
                leds[LEDdrive2][0] -= UPDATE_STEP;
                if (leds[LEDdrive2][0] < 38) {
                  leds[LEDdrive2][0] = OFF;
                  sigFlash[FLASH_DONE] = true;                                          // Flashing lite reached dark
                  if (debug) {Serial.println(dbText+"[1408] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                  flashTime = millis();
                }
              }
            break;

            case SIG_CLEAR_W_CAUTION:
              if (sigFlash[FLASH_DONE]) {
                leds[LEDdrive1][2] += UPDATE_STEP;
                leds[LEDdrive2][1] += UPDATE_STEP;
                if (leds[LEDdrive1][2] > 217) {
                  leds[LEDdrive1][2] = ON;
                  leds[LEDdrive2][1] = ON;
                  sigFlash[FLASH_DONE] = false;                                         // Flashing lite not reached dark
                  if (debug) {Serial.println(dbText+"[1422] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                  flashTime = millis();
                }
              }

              else {
                leds[LEDdrive1][2] -= UPDATE_STEP;
                leds[LEDdrive2][1] -= UPDATE_STEP;
                if (leds[LEDdrive1][2] < 38) {
                  leds[LEDdrive1][2] = OFF;
                  leds[LEDdrive2][1] = OFF;
                  sigFlash[FLASH_DONE] = true;                                          // Flashing lite reached dark
                  if (debug) {Serial.println(dbText+"[1434] - sigFlash[FLASH_DONE] set "+sigFlash[FLASH_DONE]);}
                  flashTime = millis();
                }
              }
            break;
          }
        }

//        if (debug) {Serial.println(dbText+"[1442] - Ledriver 0-1 "+leds[0][0]+","+leds[0][1]+","+leds[0][2]+","+leds[1][0]+","+leds[1][1]+","+leds[1][2]);}
//        if (debug) {Serial.println(dbText+"[1443] - Ledriver 2-3 "+leds[2][0]+","+leds[2][1]+","+leds[2][2]+","+leds[3][0]+","+leds[3][1]+","+leds[3][2]);}
        FastLED.show();
      }
    break;

    case INITIAL_SET:                                                                   // Set All signals in Stop
      if (debug) {Serial.println(dbText+"[1449] - Initial set");}

      switch (signalType[SIGNAL_FWRD]) {
        case FSI2:
        case FSI3:
          ownSigAspect[SIGNAL_FWRD] = SIG_CLEAR_W_STOP;
          oldSigAspect[SIGNAL_FWRD] = SIG_CLEAR_W_STOP;
          sigFlash[DO_FLASH] = true;
          if (debug) {Serial.println(dbText+"[1457] - Signal forward is Fsi");}
        break;

        case SIG_NOTDEF:
          // no action
          if (debug) {Serial.println(dbText+"[1462] - Signal not configured");}
        break;

        default:
          if (debug) {Serial.println(dbText+"[1466] - Signal forward is Hsi");}
          for (int i = 0; i < 10; i++) {
            FastLED.show();
            leds[0][1] += UPDATE_STEP;
            if (leds[0][1] > 217) {
              leds[0][1] = ON;
              break;
            }
            delay(100);
          }
        break;
      }

      switch (signalType[SIGNAL_BCKW]) {
        case FSI2:
        case FSI3:
          ownSigAspect[SIGNAL_BCKW] = SIG_CLEAR_W_STOP;
          oldSigAspect[SIGNAL_BCKW] = SIG_CLEAR_W_STOP;
          sigFlash[DO_FLASH] = true;
          if (debug) {Serial.println(dbText+"[1485] - Signal backward is Fsi");}
        break;

        case SIG_NOTDEF:
          // no action
          if (debug) {Serial.println(dbText+"[1490] - Signal backward not configured");}
        break;

        default:
          if (debug) {Serial.println(dbText+"[1494] - Signal backward is Hsi");}
          for (int i = 0; i < 10; i++) {
            FastLED.show();
            leds[2][1] += UPDATE_STEP;
            if (leds[2][1] > 217) {
              leds[2][1] = ON;
              break;
            }
            delay(100);
          }
        break;
      }
      if (debug) {Serial.println(dbText+"[1506] - ownSigAspect[SIGNAL_FWRD] = "+sigAspects[ownSigAspect[SIGNAL_FWRD]]);}
      if (debug) {Serial.println(dbText+"[1507] - ownSigAspect[SIGNAL_BCKW] = "+sigAspects[ownSigAspect[SIGNAL_BCKW]]);}
      if (debug) {Serial.println(dbText+"[1508] - oldSigAspect[SIGNAL_FWRD] = "+sigAspects[oldSigAspect[SIGNAL_FWRD]]);}
      if (debug) {Serial.println(dbText+"[1509] - oldSigAspect[SIGNAL_BCKW] = "+sigAspects[oldSigAspect[SIGNAL_BCKW]]);}
      if (debug) {Serial.println(dbText+"[1510] - sigFlash[DO_FLASH] = "+sigFlash[DO_FLASH]);}
      if (debug) {Serial.println(dbText+"[1511] - sigFlash[FLASH_DONE] = "+sigFlash[FLASH_DONE]);}
      if (debug) {Serial.println(dbText+"[1512] - sigFading[NEW_ASPECT] = "+sigFading[NEW_ASPECT]);}
      if (debug) {Serial.println(dbText+"[1513] - sigFading[OLD_ASPECT] = "+sigFading[OLD_ASPECT]);}

    break;

    case START_UP:                                                                      // Do LED start-up procedure
      if (debug) {Serial.println(dbText+"[1518] - Start up");}

      for (int i = 0; i < NUM_LED_DRIVERS; i++) {leds[i] = CRGB::Black;}                // Set all LED dark

      for (int k = 0; k < 4; k++) {                                                     // Lit one LED at time in sequence
        for (int i = 0; i < 3; i++) {
          leds[k][i] = ON;
          FastLED.show();
          delay(flashTimes * 250);
          leds[k][i] = OFF;
        }
      }
      for (int i = 0; i < NUM_LED_DRIVERS; i++) {leds[i] = CRGB::Black;}                // Set all LED dark

      for (int k = 0; k < 2; k++) {                                                     // Flash all LED
        for (int i = 0; i < NUM_LED_DRIVERS; i++) {leds[i] = CRGB::White;}              // Set all LED lit
        FastLED.show();
        delay(flashTimes * 250);

        for (int i = 0; i < NUM_LED_DRIVERS; i++) {leds[i] = CRGB::Black;}              // Set all LED dark
        FastLED.show();
        delay(flashTimes * 250);
      }
    break;
  }
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Main program loop
// ------------------------------------------------------------------------------------------------------------------------------
void loop() {

  // Check connection to the MQTT broker. If no connection, try to reconnect
  if (needMqttConnect) {
    if (mqttConnect()) {
      needMqttConnect = false;
    }
  }

  else if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!mqttClient.connected())) {
    if (debug) {Serial.println(dbText+"MQTT reconnect");}
    mqttConnect();
  }

  // Run repetitive jobs
  mqttClient.loop();                          // Wait for incoming MQTT messages
  iotWebConf.doLoop();                        // Check for IotWebConfig actions
  irSensor.update();                          // Check for IR-sensor actions

  if ((millis() - currTime) >= 100) {         // 0,1 sec signal check
    currTime = millis();
    updateSignals(RUNNING);                   // Update the signals
  }
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function to download config file
// ------------------------------------------------------------------------------------------------------------------------------
bool getConfigFile() {

// http://configserver.000webhostapp.com/?name=BS-1

//  http.beginRequest();
  http.get(configPath);
//  http.sendHeader("Host",cHost);
//  http.endRequest();
  int statusCode = http.responseStatusCode();
  String response = http.responseBody();
  /*
    Typical response is:
    {
      "name":"BS-1",
      "mqtt": {
        "MqttServer":"192.168.1.8",
        "MqttPort":1883,
        "MqttRootTopic":"mqtt_n"
      },
      "config": {
        "SignalForwardId":"bs-2",
        "SignalForwardDistance":"3.5",
        "SignalBackwardId":"cda2",
        "SignalBackwardDistance":2
      }
    } 
 */
  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
//  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonDocument doc(2048);

  if (debug) {Serial.println("Status Code: "+statusCode);}
  if (debug) {Serial.println("Response: "+response);}
  // Parse JSON object
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    if (debug) {Serial.print(F("deserializeJson() failed: "));}
    if (debug) {Serial.println(error.c_str());}
    return false;
  }
  else {

    // Extract values
    if (debug) {Serial.println("Response:");}
    if (debug) {Serial.println(doc["name"].as<char*>());}
    if (debug) {Serial.println("mqtt:");}
    if (debug) {Serial.println(doc["mqtt"]["MqttServer"].as<char*>());}
    if (debug) {Serial.println(doc["mqtt"]["MqttPort"].as<char*>());}
    if (debug) {Serial.println(doc["mqtt"]["MqttRootTopic"].as<char*>());}
    if (debug) {Serial.println("config:");}
    if (debug) {Serial.println(doc["config"]["SignalForwardId"].as<char*>());}
    if (debug) {Serial.println(doc["config"]["SignalForwardDistance"].as<float>(), 1);}
    if (debug) {Serial.println(doc["config"]["SignalBackwardId"].as<char*>());}
    if (debug) {Serial.println(doc["config"]["SignalBackwardDistance"].as<float>(), 1);}

    if (String(doc["name"].as<char*>()) == deviceID) {
      nodeConfig[MQTT_CONFIG][0] = String(doc["mqtt"]["MqttServer"].as<char*>());
      nodeConfig[MQTT_CONFIG][1] = String(doc["mqtt"]["MqttPort"].as<char*>());
      nodeConfig[MQTT_CONFIG][2] = String(doc["mqtt"]["MqttRootTopic"].as<char*>());
      nodeConfig[NODE_CONFIG][0] = String(doc["config"]["SignalForwardId"].as<char*>());
      nodeConfig[NODE_CONFIG][1] = String(doc["config"]["SignalForwardDistance"].as<float>());
      nodeConfig[NODE_CONFIG][2] = String(doc["config"]["SignalBackwardId"].as<char*>());
      nodeConfig[NODE_CONFIG][3] = String(doc["config"]["SignalBackwardDistance"].as<float>());
      return true;
    }
  }

  return false;
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function to set-up all topics
// ------------------------------------------------------------------------------------------------------------------------------
void setupTopics() {

  if (debug) {Serial.println(dbText+"Topics setup");}

  // Subscribe
  // Topic                körning/node/rapporttyp/id/state
  subTopic[MODULE_BCKW] = mqttRootTopic+"/"+moduleName[MODULE_BCKW]+"/#";
  subTopic[MODULE_FWRD] = mqttRootTopic+"/"+moduleName[MODULE_FWRD]+"/#";

  // Publish - device
  pubTopic[0]           = mqttRootTopic+"/"+deviceID+"/$name";
  pubTopicContent[0]    = deviceName;
  pubTopic[1]           = mqttRootTopic+"/"+deviceID+"/$nodes";
  pubTopicContent[1]    = deviceID;
  pubTopic[2]           = mqttRootTopic+"/"+deviceID+"/$type";
  pubTopicContent[2]    = "Blocksignalmodul för enkelspår";

  // Publish - node 01
  pubTopic[3]           = mqttRootTopic+"/"+deviceID+"/signal/$name";
  pubTopicContent[3]    = SIG_ONE_ID,SIG_TWO_ID;
  pubTopic[4]           = mqttRootTopic+"/"+deviceID+"/signal/$type";
  pubTopicContent[4]    = "Signal";
/*
  // Publish - node 01 - property 01
  pubTopic[5]           = mqttRootTopic+"/"+deviceID+"/signal/$properties";
  for (int i=0; i < nbrSigAspects; i++) {
    pubTopicContent[5] += sigAspects[i];
    if (i < nbrSigAspects-1) {pubTopicContent[5] += ",";}
  }

  // Publish - node 02
  pubTopic[6]           = mqttRootTopic+"/"+deviceID+"/block/$name";
  pubTopicContent[6]    = SIG_ONE_ID,SIG_TWO_ID;
  pubTopic[7]           = mqttRootTopic+"/"+deviceID+"/block/$type";
  pubTopicContent[7]    = "Block";

  // Publish - node 02 - property 01
  pubTopic[8]           = mqttRootTopic+"/"+deviceID+"/block/$properties";
  for (int i=0; i < nbrBloStates; i++) {
    pubTopicContent[8] += bloStates[i];
    if (i < nbrBloStates-1) {pubTopicContent[8] += ",";}
  }

  // Publish - node 03
  pubTopic[9]           = mqttRootTopic+"/"+deviceID+"/traffic/$name";
  pubTopicContent[9]    = "TOP_DIR";
  pubTopic[10]          = mqttRootTopic+"/"+deviceID+"/traffic/$type";
  pubTopicContent[10]   = "Tågriktning";

  // Publish - node 03 - property 01
  pubTopic[11]          = mqttRootTopic+"/"+deviceID+"/traffic/$properties";
  for (int i=0; i < nbrDirStates; i++) {
    pubTopicContent[11] += dirState[i];
    if (i < nbrDirStates-1) {pubTopicContent[11] += ",";}
  }
*/
  // Other used publish topics
  pubSigTopic[SIGNAL_FWRD]  = mqttRootTopic+"/"+deviceID+"/signal/up/"+SIG_ONE_ID+"/state"; // Own signal forward
  pubSigTopic[SIGNAL_BCKW]  = mqttRootTopic+"/"+deviceID+"/signal/up/"+SIG_TWO_ID+"/state"; // Own signal backward
  pubBloTopic[BLO_ONE]      = mqttRootTopic+"/"+deviceID+"/block/up/"+BLO_ONE_ID+"/state";  // Own block sensor state on module
  pubDirTopic               = mqttRootTopic+"/"+deviceID+"/traffic/up/"+TOP_DIR+"/state";   // Traffic direction
  pubDeviceStateTopic       = mqttRootTopic+"/"+deviceID+"/$state";                         // Node state
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Prepare MQTT broker and define function to handle callbacks
// ------------------------------------------------------------------------------------------------------------------------------
void setupBroker() {

  tmpMqttServer = mqttServer.c_str();

  if (debug) {Serial.println(dbText+"MQTT setup");}

  mqttClient.setServer(tmpMqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  if (debug) {Serial.println(dbText+"Setup done!");}
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function to show AP web start page
// ------------------------------------------------------------------------------------------------------------------------------
void handleRoot() {

  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal()) {

    // -- Captive portal request were already served.
    return;
  }

  // Assemble web page content
  String page  = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" ";
         page += "content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";

         page += "<title>MQTT-inst&auml;llningar</title></head><body>";
         page += "<h1>Inst&auml;llningar</h1>";

         page += "<p>V&auml;lkommen till MQTT-enheten med namn: '";
         page += cfgDeviceId;
         page += "'</p>";

         page += "<p>P&aring; sidan <a href='config'>Inst&auml;llningar</a> ";
         page += "kan du best&auml;mma hur just denna MQTT-klient ska fungera.";
         page += "</p>";

         page += "<p>M&ouml;jligheten att &auml;ndra dessa inst&auml;llningar &auml;r ";
         page += "alltid tillg&auml;nglig de f&ouml;rsta 30 sekunderna efter start av enheten.";
         page += "</p>";

         page += "</body></html>\n";

  // Show web start page
  server.send(200, "text/html", page);
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function beeing called when wifi connection is up and running
// ------------------------------------------------------------------------------------------------------------------------------
void wifiConnected() {

//  long rssi = wifiClient.RSSI();
//  if (debug) {Serial.println(dbText+"Signal strength (RSSI): "+rssi+" dBm");}

  String pathID = cPath+deviceID;
  configPath = pathID.c_str();
  if (debug) {Serial.println(dbText+"configPath :"+configPath);}

  if (getConfigFile()) {
    mqttServer                  = nodeConfig[MQTT_CONFIG][0];         // MqttServer
    mqttPort                    = nodeConfig[MQTT_CONFIG][1].toInt(); // MqttPort
    mqttRootTopic               = nodeConfig[MQTT_CONFIG][2];         // MqttRootTopic
    moduleName[MODULE_FWRD]     = nodeConfig[NODE_CONFIG][0];         // SignalForwardId
    moduleDistance[MODULE_FWRD] = nodeConfig[NODE_CONFIG][1];         // SignalForwardDistance
    moduleName[MODULE_BCKW]     = nodeConfig[NODE_CONFIG][2];         // SignalBackwardId
    moduleDistance[MODULE_BCKW] = nodeConfig[NODE_CONFIG][3];         // SignalBackwardDistance
  }

  // We are ready to start the MQTT connection
  needMqttConnect = true;
}


// ------------------------------------------------------------------------------------------------------------------------------
//  Function that gets called when web page config has been saved
// ------------------------------------------------------------------------------------------------------------------------------
void configSaved() {

  if (debug) {Serial.println(dbText+"IotWebConf config saved");}
  deviceID                = String(cfgDeviceId);
  deviceID.toLowerCase();
  deviceName              = String(cfgDeviceName);
  mqttPort                = atoi(cfgMqttPort);
  mqttServer              = String(cfgMqttServer);
  moduleName[MODULE_BCKW] = String(cfgSignalm1Id);
  moduleName[MODULE_FWRD] = String(cfgSignalm2Id);
  for (int i = 0; i< nbrSigTypes; i++) {
    if (signalTypes[i] == String(cfgSignal1Type)) {signalType[SIGNAL_BCKW] = i;}
    if (signalTypes[i] == String(cfgSignal2Type)) {signalType[SIGNAL_FWRD] = i;}
  }
  mqttRootTopic           = String(cfgMqttTopic);
  ledBrightness           = atoi(cfgLedBrightness);

  FastLED.setBrightness(ledBrightness);
}

// ------------------------------------------------------------------------------------------------------------------------------
//  Function to validate the input data form
// ------------------------------------------------------------------------------------------------------------------------------
bool formValidator() {

  if (debug) {Serial.println(dbText+"IotWebConf validating form");}
  bool valid = true;
  String msg = "Endast dessa är tillåtna signaltyper: ";

  for (int i = 0; i < nbrSigTypes - 2; i++) {
    if (signalTypes[i] ==  server.arg(webSignal1Type.getId())) {
      valid = true;
      break;
    }

    else {
      valid = false;
      msg += signalTypes[i];
      if (i == nbrSigTypes -1){msg += ",";}
    }
  }

  if (!valid) {
    webSignal1Type.errorMessage = "Endast dessa är tillåtna signaltyper: Not used, Hsi2, Hsi3, Hsi4, Hsi5";
  }

  return valid;
}
