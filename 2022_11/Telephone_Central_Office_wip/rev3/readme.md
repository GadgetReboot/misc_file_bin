New features in this revision:<br>
-Focus on using the RS485 serial output pins (Serial.swap to stop using usb debug serial) so the sketch is really only looking for a single character to indicate incoming call.<br>
-Added buggy states so state machine can place a call to a hard coded phone number or receive a call and connect the two nodes with the secondary phone cable
<br><br>
The main goal was to exercise the hardware functionality as a proof of concept and after deciding on some infrastructure changes, such as attempting to migrate to ESP32, this ESP8266 prototype development may end.<br><br>
video playlist: https://www.youtube.com/watch?v=m8xOpIFWfZg&list=PLjJ5BDlAoGarnVThQQigLQh1viLgSvlCM
