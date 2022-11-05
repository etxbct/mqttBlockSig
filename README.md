# mqttBlockSig
Is a Swedish block signal module.

In Sweden when double track is used one track is the up track (uppspår) and the other track is down track (nedspår).
If facing the module from backside the nearest track is the down track (nedspår) carrying traffic from right to left.
The furthest track is the up track (uppspår) carrying traffic from left to right.
If single track is used then the track is the up track (uppspår).

For train direction "up" is when trains are going in default traffic direction.
For train Direction "down" is when trains are going in oposite direction.
So for a single track; if train is going from left to right it is having the traffic direction "up".

### Topics
Topic used:

	type        root    /node   /report type    /track  /name       /state
	Signals   - mqtt_n  /bs-1   /signal         /up     /cda1       /state
	Blocks    - mqtt_n  /bs-1   /block          /up     /s123       /state
	Turnouts  - mqtt_n  /cda    /turnout        /up     /vx23       /state
	Traffic   - mqtt_n  /bs-1   /traffic        /up     /direction  /state
	Traffic   - mqtt_n  /bs-1   /traffic        /up     /train      /id


	root       : name of MQTT root.
	node       : name of the MQTT node.
	report type: type of reporter.
	track      : up or down.
	name       : name of the reporter.
	state      : reported state.
	id         : train id.

### Payloads
State payloads for signals:

	Main signal (huvudsignal)             : {stop,d40,d80,d40short}
	Main dwarf signal (Huvuddvärgsignal)  : {stop,d40,d80,d40v,d80v}
	Distant signal (försignal)            : {d80wstop,d80wd40,d80wd80}
	Dwarf signal (dvärgsignal)            : {stop,rt,rtv,rtf}

State payload for other objects:

	Turnouts                              : {closed,thrown}
	Blocks                                : {free,occupied}
	Direction                             : {up,down}

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

