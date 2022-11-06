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


### Topics
Topic used for signals, blocks and turnouts:

	type        root    /client   /report type  /exit   /track  /item       /state
	Signals   - mqtt_n  /bs-1     /signal       /a      /up     /cda1       /state
	Blocks    - mqtt_n  /bs-1     /block        /b      /up     /s123       /state
	Turnouts  - mqtt_n  /cda      /turnout      /a      /up     /vx23       /state
	Traffic   - mqtt_n  /bs-1     /traffic      /a      /up     /direction  /state
	Traffic   - mqtt_n  /bs-1     /traffic      /b      /up     /train      /id

	root       : name of MQTT root.
	client     : name of the MQTT client.
	report type: type of reporter.
	exit       : 
	track      : up or down.
	item       : name of the reporter.
	state      : reported state.
	id         : train id.

### Payloads
Payloads for signals:

	Main signal (huvudsignal)           : {stop,d40,d80,d40short}
	Main dwarf signal (Huvuddvärgsignal): {stop,d40,d80,d40v,d80v}
	Distant signal (försignal)          : {d80wstop,d80wd40,d80wd80}
	Dwarf signal (dvärgsignal)          : {stop,rt,rtv,rtf}

Payload for other report types:

	Turnout                             : {closed,thrown}
	Block                               : {free,occupied}
	Traffic/direction                   : {up,down}
	Traffic/train                       : id (schedule number for the train)

### Signal aspects

	payload     Main signal                     Main Dwarf signal       Distant signal      Dwarf signal
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

