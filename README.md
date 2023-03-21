# mqttBlockSig
Is a Swedish block signal module.

The Signal module can have in total four exits, A-D.
The left side has A and C and the right side has B and D.

In Sweden when double track is used one track is the up track (uppspår) and the other track is down track (nedspår).
If facing the module from backside the nearest track is the down track (nedspår) carrying traffic from right to left, where left exit is A and right exit is B.
The furthest track is the up track (uppspår) carrying traffic from left to right, where left exit is C and right exit is D.
If single track is used then the track is the up track (uppspår).

For train direction *up* is when trains are going in default traffic direction.
For train Direction *down* is when trains are going in oposite direction.
So for a single track; if train is going from left to right it is having the traffic direction *up*.

## Message Types
There are two basic types of messages that can be used: "command" and "data".

### Data messages

The "data" type messages are used to report information from one node to other nodes concerning the state of "something".
The data messages are intended to be one-to-many.

The "something" could be a signal: "signal block-1 is now set to approach". Or it could be a turnout: "turnout steel-mill has been closed".

The "something" could also be a "ping" message proclaiming that the node is alive and running well.

Data type message topics always begin with the prefix "dt":

- "dt / `<scale>` / `<type>` / `<node-id>` / `<port_id>`" - data message

## Command messages

The other broad class of messages are "commands". These messages are intended to be one-to-one type messages. One node requests another node perform an operation. "Set signal block-1 to approach". The subscribing node processes the command request and publishes a response. "Signal 123 has been set to approach".

>Since MQTT topics are being used to control message flow, the requesting node does not really know which actual node will respond to the command request. Any physical node could have subscribed to the topic in question.

Command message topics begin with the prefix of "cmd". Whether a "command" message is a request or response is indicated by the last term of the topic:

- "cmd / `<scale>` / `<type>` / `<node-id>` / `<port_id>` / req" - command request
- "cmd / `<scale>` / `<type>` / `<node-id>` / `<port_id>` / res" - command response

>Response? Why do nodes need to send a response?
Please remember that MQTT is a distributed message system. The node making a request does not directly communicate with the node that will perform the operation. All messaging is indirect via a broker computer. Without a response mechanism, the requestor does not know if its request was even received let alone successfully carried out.

>MQTT does have a built in mechanism to ensure messages are delivered. It is called Quality if Service (QOS). It is a mechanism handled in the low level MQTT client library code. Depending which client library you are using you may see different levels of support for QOS. The lowest level of QOS, level 0 is fine to use for MQTT-LCP.

### Session ID

Two particular parts of the request body are very import in the request/response pair. First, the request body must contain a session id that requestor uses to match responses to outstanding request. A requestor may have many outstanding requests at one time. Imaging a throttle requesting a certain route. If there are six turnouts in the route, the throttle may issue six different switch requests. Having the same session id in the response that was in the request allows the throttle to know which switches were changed by matching responses to requests.

### Respond To

The other important part of the request is the "respond-to". It is used by the responder as the topic of the response it publishes to the requestor. In snail-mail terms, it is the return address or in email it is the “from”.

It is recommended that the response topic be the in this form:

Topic: `cmd/h0/node/node-id`

To receive the responses, the requesting node would subscribe to:

Subscribe: `cmd/h0/node/<node-id>/#`

Thereafter no matter what kind of request (track, cab,switch) was made by the requestor node, the responses could be received at one point in app.

### Used cmd by mqttBlockSig

The only used cmd topic used is to send the inventory. The mqtt-registry sends a report/req command to every MQTT node to report its inventory.

### Topics
Topic used for signals, blocks and turnouts:

	type        msg-type /scale /type    /node-id /port-id
	Signals   - dt       /h0    /signal  /bs-1    /a-out
	Blocks    - dt       /h0    /sensor  /bs-1    /s1
	Turnouts  - dt       /h0    /switch  /cda     /vx21
	Traffic   - dt       /h0    /traffic /bs-1    /a-out

	msg-type   : dt or cmd.
	scale      : track scale.
	type       : The type of message.
	node-id    : The name of the application node.
	port-id    : Optional identifier for a port controlled by a node. 

### Payload
MQTT message topics are used to control the routing of the messages.
The MQTT message body, on the other hand, contains the contents of the message.

The body is a JSON formatted message body. Each message body's JSON conforms to a format suggested by the [Amazon AWS Whitepaper](https://d1.awsstatic.com/whitepapers/Designing_MQTT_Topics_for_AWS_IoT_Core.pdf).

There are some common elements in all JSON message bodies:

- root-element : Type of message ("sensor", "ping", "switch", etc), matches topic type in the message topic.
- "version" : version number of the JSON format
- "timestamp" : time message was sent in seconds since the unix epoch (real time, not fastclock)
- "session-id" : a unique identifier for the message.

>Exception: The session-id in a response must match the session-id of the corresponding request message.

- "node-id" : The application node id to receive the message. Matches node-id in message topics.
- "port-id" : The port on the receiving node to which the message is to be applied. Matches port-id in the message topic
- "identity" : Optional. The identification of a specific item like a loco or rfid tag.
- "respond-to" : " Message topic to be used in the response to a command request. The "return address".
- "state" : The state being requested to be changed or the current state being reported.
	- State has two sub elements:
		- desired : The state to which the port is to be changed: "close", "off", etc. Used in command type messages
		- reported : The current state of a port: "closed", "off", etc. Used command response and data messages
- "metadata" : Optional. Varies by message type.

Reported state for signals:

	Main signal (huvudsignal)           : "state": {"reported": "stop" or "d40" or "d80" or "d40short"}
	Main dwarf signal (Huvuddvärgsignal): "state": {"reported": "stop" or "d40" or "d80" or "d40v" or "d80v"}
	Distant signal (försignal)          : "state": {"reported": "d80wstop" or "d80wd40" or "d80wd80"}
	Dwarf signal (dvärgsignal)          : "state": {"reported": "stop" or "rt" or "rtv" or "rtf"}

Reported state for other report types:

	switch                              : "state": {"reported": "closed" or "thrown"}
	sensor                              : "state": {"reported": "free" or "occupied"}
	traffic                             : "state": {"reported": "out" or "in"}

### Signal aspects

	reported    Main signal                     Main Dwarf signal       Distant signal      Dwarf signal
	stop        red                             red                     -                   two white vertically
	d40         two green                       left green              -                   -
	d80         one green                       right green             -                   -
	d40short    three green                     -                       -                   -
	d40v        -                               left flashing green     -                   -
	d80v        -                               right flashing green    -                   -
	d80wstop    one green, one flashing green   -                       one flashing green  -
	d80wd40     one green, two flashing green   -                       two flashing green  -
	d80wd80     one green, one flashing white   -                       one flashing white  -
	rt          -                               -                       -                   two white horizontal
	rtv         -                               -                       -                   two white left diagonal
	rtf         -                               -                       -                   two white right diagonal

### Example of messages

Topic: `dt/h0/ping/bs-1`
Body: `{"ping": {"version":"1.0", "session-id":"dt:1605207768", "timestamp":1605207768,
"node-id":"bs-1", "state": {"reported":"ping"}}}`

Topic: `dt/h0/signal/bs-1/block-1`
Body: `{"signal": {"version":"1.0", "timestamp":1590344126, "session-id":"dt:1590344126",
"node-id":"bs-1", "port-id":"block-1", "state": {"reported":"d80"}}}`

Topic: `dt/h0/sensor/bs-1/block-1`
Body: `{"sensor": {"version":"1.0", "timestamp":1590339121, "session-id":"dt:1590339121",
"node-id":"bs-1", "port-id":"block-1", "identity":"1234",
"state": {"reported":"occupied"},
"metadata": {"type": "railcom", "facing":"b-out"}}}`

Topic: `cmd/h0/node/bs-1/report/req`
Body: `{"inventory": {"version":"1.0", "timestamp":1680635134, "session-id":"req:1680635134",
"respond-to":"cmd/h0/node/mqtt-registry/res",
"node-id":"mqtt-registry",
"state": {"desired": {"report": "inventory"}}}}`

Topic: `cmd/h0/node/mqtt-registry/res`
Body: `{"inventory": {"version":"1.0", "timestamp":1680635193, "session-id":"req:1680635134",
"node-id":"bs-1",
"state": {"desired": {"report": "inventory"}, "reported": {"report": "inventory"}},
"metadata": {...}}}`
