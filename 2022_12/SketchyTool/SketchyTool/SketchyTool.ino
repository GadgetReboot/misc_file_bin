/*
   Sketchy Tool - I2C interface to pass data to another project for on the fly variable tweaking

   Hardware: ATTiny 1604
             I2C (bit banged) 128x64 OLED display
             Rotary Encoder with button for setting variable values
             I2C bus (hardware serial) to send variable data to target project

             Pinout
             OLED SCL = PA3  bit banged
             OLED SDA = PA2  bit banged
             Rotary Encoder Input A = PA4
             Rotary Encoder Input B = PA5
             Rotary Encoder Switch  = PA6
             External SCL = PB0
             External SDA = PB1

   Tested with Arduino IDE 1.8.13
               MegaTinyCore board file 2.5.10

   Gadget Reboot
   https://www.youtube.com/gadgetreboot

*/

// OLED and bit banged I2C bus controller
#include "Bb7654Oled.h"
#include "PixOled.h"
#include "TerOled.h"

// I2C hardware serial bus
#include <Wire.h>

const byte slaveAddr = 0x42;   // I2C address for slave (other project board to send data to)

// Rotary Encoder Inputs
const byte encA  = 0;          // rotary encoder input A  PA4
const byte encB  = 1;          // rotary encoder input B  PA5
const byte encSW = 2;          // rotary encoder switch   PA6

// encoder input A state for tracking encoder movements
int encAState;
int encALastState;

unsigned long buttonTimer = 0;      // encoder button debounce timer

// timer for main loop to periodically send data to I2C slave
unsigned long pingTimer = millis();
unsigned long pingTimerVal = 5000;  // mS

void setup() {

  // init OLED with bit banged I2C
  SetupBb7654Oled();
  SetupPixOled();
  DoubleH();
  Clear();

  // print header text on OLED
  LiCol(0, 0); Text("Rotate and Click");

  // init hardware serial I2C communications as master
  Wire.begin();

  // configure encoder pins as inputs
  pinMode(encA, INPUT_PULLUP);
  pinMode(encB, INPUT_PULLUP);
  pinMode(encSW, INPUT_PULLUP);

  // read the initial state of encA to track movements
  encALastState = digitalRead(encA);
}


void loop() {

  readEncoder();                 // monitor rotary encoder inputs for activity

  // periodically update the slave device with a default LED blink rate
  // so any changes sent with the rotary encoder button are reset later
  if ((millis() - pingTimer) > pingTimerVal) {
    pingTimer = millis();
    byte blinkVal = 10;          // 0 to 255 can be sent to slave device to control LED blink rate
    sendI2CData(blinkVal);       // send data to the slave device
  }

}

// send data to I2C slave
void sendI2CData(byte value) {
  Wire.beginTransmission(slaveAddr);
  Wire.write(value);
  Wire.endTransmission();
}


void readEncoder() {
  encAState = digitalRead(encA);   // read current state of encA
  // if encA has changed state since the last reading, movement has occurred
  if (encAState != encALastState) {
    // if encB is different from encA, clockwise rotation has occurred
    if (digitalRead(encB) != encAState) {
      LiCol(1, 0);
      Text("CW   ");
    }
    else {
      LiCol(1, 0);
      Text("CCW  ");
    }
  }
  encALastState = encAState; // update the last state of encA to the current state

  // read the encoder switch
  int encSwState = digitalRead(encSW);

  // if the switch is pressed, debounce
  // and send new data to the I2C slave
  if (encSwState == LOW) {
    // check if it's been at least 50mS since the 
    // last time the switch was read as being pressed (low) 
    // to respond to initial button presses and ignore again until debounced
    if (millis() - buttonTimer > 50) {
      LiCol(1, 0);  Text("Click");
      sendI2CData(255);      // send LED slow blink delay value to I2C slave
      delay(50);
    }
    buttonTimer = millis();  // reset button debounce timer every time a pressed (low) state is read
  }
}

// Reference examples for other OLED commands
//  LiCol(1,0);Bin8(36); Hex16(0x1234); Dec16(19999);
//  LiCol(2,0);Bin16(vals); // 4.8ms
//  LiCol(5,0);Dec16(valss);Dec16(-valss);
//  LiCol(6,0);Dec16(vals);Dec16(-vals);
//  LiCol(7,0);Text("done");
