/*
   I2C Slave Control Demo

   The Uno blinks an LED at a default "slow" rate
   If the "Sketchy Tool" is connected to the I2C bus,
   the blink rate is controlled by received bytes to
   demonstrate using the sketchy tool to perform
   real time variable tweaking on another project's sketch.

   Target Hardware:  Uno

   Tested with Arduino IDE 1.8.13

   Gadget Reboot
   https://www.youtube.com/gadgetreboot

*/

// I2C setup
#include <Wire.h>
const byte slaveAddr = 0x42;   // I2C address for this device

volatile byte tweakData = 255; // tweak variable for data received over I2C
int ledBlinkDelay = 255;       // led blink rate control variable

void setup() {

  Serial.begin(115200);
  Serial.println("\r\nI2C Slave Demo - LED blink rate controlled by Sketchy Tool");

  pinMode(LED_BUILTIN, OUTPUT); // on board LED blinks at the requested rate

  // join I2C bus with slaveAddr
  Wire.begin(slaveAddr);

  // call this function when I2C data received
  Wire.onReceive(receiveEvent);

}

void loop() {
 
  // map LED blink rate from an I2C tweak data range of 0 to 255 
  // over to an on and off time between 100mS and 2000mS 
  ledBlinkDelay = map(tweakData, 0, 255, 100, 2000);

  // blink LED based on tweak data received over I2C
  digitalWrite(LED_BUILTIN, HIGH);
  delay(ledBlinkDelay);
  digitalWrite(LED_BUILTIN, LOW);
  delay(ledBlinkDelay);

}

void receiveEvent() {
  tweakData = Wire.read();    // read one I2C byte
  Serial.print("I2C tweak byte received: ");
  Serial.println(tweakData);
}
