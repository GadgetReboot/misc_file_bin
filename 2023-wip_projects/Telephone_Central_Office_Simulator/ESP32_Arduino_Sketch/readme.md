Using:<br>
Arduino IDE 1.8.13<br>
ESP32 board file 1.0.6<br><br>
Porting the sloppy residual ESP8266 code over and getting basic WiFi going.<br><br>
Features/Notes:<br>
Rev1:<br>
There is an ESP32 jumper on GPIO35 to configure whether the node is a wifi Sender or Server.<br>
If the pin is high, the module is configured as a wifi Sender.  A low pin is a wifi Server.<br>
The same sketch will go into two different nodes (ESP32 modules with all the phone interface hardware etc) and based on the jumper config, one will become the Server (master) while the other is a Sender (slave).<br>
The difference is only the Server node will create a web server to allow access for future features including monitoring system status and changing settings on nodes.<br>
Another difference is this Server/Sender relationship is used for auto-pairing with ESP-NOW so the modules can have a wireless coordinating trunk line along with the tip/ring connection between nodes.<br> 
ESP32 pin 35 is an input only and has no internal pull-up resistor so there is an external 10K to 3.3v and a jumper to GND.<br><br>
This revision has commented out most of the old ESP8266 functionality to try to get things working from the ground up, starting with the new features (wireless trunk line and the ability to play back audio recorded messages with Mozzi, eg. the "This is a recording" message).<br>
When this sketch is run, two nodes will auto-pair using ESP-NOW, sending mock data back and forth, the Sender node will create a web page (requires joining a wifi router), and the module will play back the "this is a recording" message on the Mozzi audio output pin.
