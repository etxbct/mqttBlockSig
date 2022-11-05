# mqttBlockSig
Is a Swedish block signal module.

Topic used:
             root  /node /report type  /track  /name     /state
 Signals   - mqtt_n/bs-1 /signal       /up     /cda1     /state
 Blocks    - mqtt_n/bs-1 /block        /up     /s123     /state
 Turnouts  - mqtt_n/cda  /turnout      /up     /vx23     /state
 Traffic   - mqtt_n/bs-1 /traffic      /up     /direction/state

 root        : name of MQTT root
 node        : name of the MQTT node
 report type : type of reporter
 track       : up or down
 name        : name of the reporter. For traffic it is direction or train id

## Payloads
State payloads for signals:
	Main signal (huvudsignal)	{stop,d80,d40,d40short,d80v,d40v}
	Distant signal (försignal)	{d80wstop,d80wd80,d80wd40}
	Dwarf signal (dvärgsignal)	{stop,rt,rtv,rtf}

	payload		Main signal			Main Dwarf signal		Distant signal		Dwarf signal
	stop		red				red				-			two white vertically
	d80		one green			right green			-			-
	d40		two green			left green			-			-
	d40short	three green			-				-			-
	d80v		-				right flashing green		-			-
	d40v		-				left flashing green		-			-
	d80wstop	one green, one flashing green	-				one flashing green	-
	d80wd80		one green, one flashing white	-				one flashing white	-
	d80wd40		one green, two flashing green	-				two flashing green	-
	rt		-				-				-			two white horizontal
	rtv		-				-				-			two white left diagonal
	rtf		-				-				-			two white right diagonal

## States
State payloads for Blocks:
{free,occupied}

State payloads for Turnouts:
{closed,thrown}

State payloads for Traffic direction:
{up,down}
