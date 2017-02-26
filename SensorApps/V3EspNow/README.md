Use ESP NOW to monitor a remote battery powered sensor




GoNotify Slave		===> 	GoNotify Master						===> 	Wifi AP
Sensor						Bridge espnow controller to MQTT			Internet connectivity


To compile Arduino sketches with esp now code do the following:
Quit Arduino IDE and edit this file:
C:\Users\<username>\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\<esp8266 version>\platform.txt

Search "compiler.c.elf.libs", and append "-lespnow" at the end of the line. Then you can build it by Arduino IDE.
