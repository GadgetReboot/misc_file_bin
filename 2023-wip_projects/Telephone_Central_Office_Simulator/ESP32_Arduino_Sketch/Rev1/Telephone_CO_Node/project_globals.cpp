/*
    variables offloaded here to keep things uncluttered
*/

#include "project_globals.h"

// myRegion can be northAmerica or uk (see project_globals.h)
// uncomment the desired region to use:
 byte myRegion = northAmerica;
//byte myRegion = uk;

#include <Adafruit_PCF8574.h>          // gpio expander library

 Adafruit_PCF8574 pcf_gpio;             // gpio expander object
 const byte slic_fwdRev   = 0;          // slic forward/reverse pin on gpio expander
 const byte slic_ringMode = 1;          // slic ring mode       pin on gpio expander
 const byte slic_swHook   = 2;          // slic switch hook     pin on gpio expander
 const byte Q1 = 3;                     // mt8870 dtmf data bits to decode from gpio expander
 const byte Q2 = 4;
 const byte Q3 = 5;
 const byte Q4 = 6;
 const byte StD = 7;                    // mt8870 delayed steering = high when valid Q1..Q4 data present

 const byte senderServerPin = 35;   // gpio35 low = Server  high = Sender  mode for wifi purposes

// system status flag register to track the state of various modes and operations
 uint8_t sysFlags = 0x00;            // start with all flags 0
 const byte sysFlag_offHook  = 0;    // sysFlags bit 0:  0 = phone is on hook       1 = phone is off hook
 const byte sysFlag_ringing  = 1;    // sysFlags bit 1:  0 = phone is not ringing   1 = phone is ringing
 const byte sysFlag_dtmf     = 2;    // sysFlags bit 2:  0 = no new dtmf data       1 = new dtmf data
 const byte sysFlag_StD_trig = 3;    // sysFlags bit 3:  0 = dtmf StD has expired   1 = dtmf StD has not expired

 const byte gpioInt = 10;               // gpio expander interrupt input
 const byte ringLed = 10;               // led indicating phone is ringing
 const byte relayPin = 10;              // relay that connects remote phone line to local phone line
 const byte re_dePin = 10;              // RS485 receiver/driver enable

 char serialChar = ' ';                 // characters read from serial port will control states
 volatile bool irq_flag = false;        // flag to indicate interrupt has occurred
 unsigned long slic_timer = millis();   // used for various SLIC timing eg. ringing a phone or off hook time out
 unsigned long slic_ring_freq_timer = millis();       // used to generate the proper ring freq
 byte slic_ring_cadence_step = 0;                     // which of the cadence intervals is the ring pattern generating?
 int ringCadence [] = {0, 0, 0, 0};                   // SLIC phone ring cadence for ring/silence/ring/silence (durations in mS)
 const byte ringFreq = 45;                            // generate this freq (Hz) ring voltage (possible issues below 40Hz)
 const int ringPeriod = (1 / (float)ringFreq) * 1000; // period of ring frequency in mS
