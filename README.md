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
There are two basic types of messages that can be used: "command" and "data". Block signal uses only data messages.

### Data messages
The "data" type messages are used to report information from one node to other nodes concerning the state of "something".
The data messages are intended to be one-to-many.

The "something" could be a signal: "signal block-1 is now set to approach". Or it could be a turnout: "turnout steel-mill has been closed".

The "something" could also be a "ping" message proclaiming that the node is alive and running well.

Data type message topics always begin with the prefix "dt":

- "dt / `<scale>` / `<type>` / `<node-id>` / `<port_id>`" - data message

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

