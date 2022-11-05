/**
  * Settings for this specific MMRC client
  */
// Access point
#define APNAME                  "bs-1"
#define APPASSWORD              "block1234"
#define ROOTTOPIC               "mqtt_n"

// Debug
#define SET_DEBUG               true

// Configuration host
#define CONFIG_HOST             "configserver.000webhostapp.com"
#define CONFIG_HOST_PORT        80
#define CONFIG_HOST_FILE        "/?name=" 
#define MQTT_CONFIG             0
#define NODE_CONFIG             1

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION          "test1"

// Default string and number lenght
#define STRING_LEN             32
#define NUMBER_LEN              8

// FastLED settings
#define NUM_LED_DRIVERS         4   // Max number of signal LED drivers
#define SIG_DATA_PIN           D6   // Led signal pin 12
#define ON                    255   // Led on
#define OFF                     0   // Led off
#define LED_BRIGHTNESS        125
#define FLASH_AT_STARTUP        3   // Greater than 0 will flash all LEDs at start-up

// Job phases
#define RUNNING                 0   // Normal phase
#define INITIAL_SET           250   // Phase after Start-up
#define START_UP              255   // Start up phase

// Define which pin to use for block IR-sensor
#define SENSOR1_PIN             D7  // IR-sensor on single track or up track
#define SENSOR2_PIN             D8  // IR-sensor on down track

// Modules
#define MODULE_FWRD             0   // Module on forward track
#define MODULE_BCKW             1   // Module on backward track

// Signals
#define SIG_ONE_ID              "forward"
#define SIG_ONE_TYPE            "Hsi5"
#define SIG_TWO_ID              "backward"
#define SIG_TWO_TYPE            "Hsi5"
#define SIG_NOTDEF              0
#define HSI2                    1
#define HSI3                    2
#define HSI4                    3
#define HSI5                    4
#define FSI2                    5
#define FSI3                    6
#define FLASH_FREQ1           400   // Flash time in ms (60 bpm Hsi4-5)
#define FLASH_FREQ2           300   // Flash time in ms (80 bpm Fsi2-3)
                                    // försignal 80 bpm duty cycle 3/8 (3 delay på, 5 delay av)
                                    // äldre försignal 60 bpm
#define UPDATE_STEP           127   // Steps from 0-255 and 255-0 when setting LED to ON or OFF 

#define SIGNAL_FWRD             0   // Signal in forward direction
#define SIGNAL_BCKW             1   // Signal in backward direction

// Signal types
#define SIG_NOTUSED             "Not used"
#define SIG_HSI2                "Hsi2"
#define SIG_HSI3                "Hsi3"
#define SIG_HSI4                "Hsi4"
#define SIG_HSI5                "Hsi5"
#define SIG_FSI2                "Fsi2"
#define SIG_FSI3                "Fsi3"

// Signal aspects settings
#define SIG_STOP                0   // Hsi2-5, Hdvsi  stop
#define SIG_CLEAR               1   // Hsi2-5, Hdvsi  d80
#define SIG_CAUTION             2   // Hsi3-5, Hdvsi  d40
#define SIG_CLEAR_W_STOP        3   // Hsi4-5, Fsi2-3 d80wstop
#define SIG_CLEAR_W_CLEAR       4   // Hsi4-5, Fsi2-3 d80wd80
#define SIG_CLEAR_W_CAUTION     5   // Hsi5  , Fsi3   d80wd40
#define SIG_SHORT_ROUTE         6   // Hsi5           d40short
#define SIG_ALL_FLASH         255   // All flash
#define OLD_ASPECT              0
#define NEW_ASPECT              1
#define DO_FLASH                0
#define FLASH_DONE              1

// Signal topic settings
#define TOP_SIG_STOP            "stop"
#define TOP_SIG_CLEAR           "d80"
#define TOP_SIG_CAUTION         "d40"
#define TOP_SIG_CLEAR_W_STOP    "d80wstop"
#define TOP_SIG_CLEAR_W_CLEAR   "d80wd80"
#define TOP_SIG_CLEAR_W_CAUTION "d80wd40"
#define TOP_SIG_SHORT_ROUTE     "d40short"

// Block settings
#define BLO_FREE                1   // Block free
#define BLO_OCCU                0   // Block occupied

// Block topic settings
#define TOP_BLO_FREE            "free"
#define TOP_BLO_OCCU            "occupied"

#define BLO_ONE                 0   // Block sensor on forward track direction and on single track
#define BLO_TWO                 1   // Block sensor on backward track direction
#define LINE_FWRD               0   // Block status on forward track
#define LINE_BCKW               1   // Block status on backward track
#define BLO_ONE_ID              "s1"
#define BLO_TWO_ID              "s2"

// Train direction
#define TOP_DIR                 "direction"
#define DIR_FWRD                0   // Direction forward
#define DIR_BCKW                1   // Direction backward

// Train direction
#define TOP_DIR_FWRD            "up"
#define TOP_DIR_BCKW            "down"

// Defince which pin to use for LED output
//#define LED_PIN LED_BUILTIN

// Configuration pin
// When CONFIG_PIN is pulled to ground on startup, the client will use the initial
// password to buld an AP. (E.g. in case of lost password)
//#define CONFIG_PIN D2

// Status indicator pin
// First it will light up (kept LOW), on Wifi connection it will blink
// and when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN
