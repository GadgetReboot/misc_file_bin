/*  
    Plain Old Telephone Service (POTS) simulator 
    work in progress

    Can generate DTMF call progress tones (selectable regions) (using mozzi library)
    Uses a PCF8574 GPIO expander to interface with a subscriber line interface circuit module KS0835F SLIC
    When the phone is taken off hook, a dial tone plays
    When the phone is on hook, it can be made to ring using the SLIC module

    schematic will be available as progress continues
    Audio is played on ESP8266 GPIO2 (WeMos D1 Mini pin D4)

    Mozzi documentation/API
		https://sensorium.github.io/Mozzi/doc/html/index.html

   External Libraries Used: Adafruit PCF8574 https://github.com/adafruit/Adafruit_PCF8574
                            Mozzi https://github.com/sensorium/Mozzi

   Tested with Arduino IDE 1.8.13
               ESP8266 board file 3.0.2

   Gadget Reboot
   https://www.youtube.com/gadgetreboot
*/

#include <Adafruit_PCF8574.h>     // gpio expander library
#include "mozzi_call_progress.h"  // library to generate region specific call progress tones
#include "project_globals.h"      // definitions used throughout the project files

// myRegion can be northAmerica or uk (see project_globals.h)
// uncomment the desired region to use:
//byte myRegion = northAmerica;
byte myRegion = uk;

Adafruit_PCF8574 pcf_gpio;         // gpio expander object
#define slic_fwdRev    0           // slic forward/reverse pin on gpio expander
#define slic_ringMode  1           // slic ring mode       pin on gpio expander
#define slic_swHook    2           // slic switch hook     pin on gpio expander

const byte gpioInt = D6;           // gpio expander interrupt input
char serialChar = ' ';             // characters read from serial port will control states
volatile bool irq_flag = false;    // flag to indicate interrupt has occurred
unsigned long slic_timer = millis();  // used for various SLIC timing eg. ringing a phone or off hook time out
unsigned long slic_ring_freq_timer = millis();  // used to generate the proper ring freq
byte slic_ring_cadence_step = 0;                // which of the cadence intervals is the ring pattern generating?
int ringCadence [] = {0, 0, 0, 0};    // SLIC phone ring cadence for ring/silence/ring/silence (durations in mS)
const byte ringFreq = 45;                 // generate this freq (Hz) ring voltage
const int ringPeriod = (1 / (float)ringFreq) * 1000; // period of ring frequency in mS

// system status flag register to track the state of various modes and operations
uint8_t sysFlags = 0x00;           // start with all flags 0
const byte sysFlag_offHook = 0;    // sysFlags bit 0:  0 = phone is on hook       1 = phone is off hook
const byte sysFlag_ringing = 1;    // sysFlags bit 1:  0 = phone is not ringing   1 = phone is ringing

// create call progress tone generator based on defined region
mozzi_call_progress callProgressGen(myRegion);

void setup() {
  Serial.begin(115200);
  Serial.println("\r\nPlain Old Telephone Service simulator");
  Serial.print("Region switch: ");

  switch (myRegion) {
    case northAmerica:
      Serial.print("North America");
      break;

    case uk:
      Serial.print("United Kingdom");
      break;

    default:
      Serial.print("North America");
      break;
  }
  Serial.println();

  callProgressGen.startup();           // initialize mozzi call progress tone generator
  pcf_gpio.begin(0x20, &Wire);         // gpio expander address is hard wired to 0x20

  initSysFlags();      // check the system and set status flags
  initSlic();          // configure SLIC control pins and phone ring cadence based on set region

  // interrupt input for gpio expander input activity monitoring
  pinMode(gpioInt, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(gpioInt), isr, FALLING);

  pinMode(D0, OUTPUT);  // *********Debug use an LED on port to indicate phone hook status**********
  digitalWrite(D0, 1);
} // end setup

void loop() {

  // check if gpio expander interrupt has occurred and perform actions
  if (irq_flag) {
    irq_flag = false;         // clear interrupt flag

    if (pcf_gpio.digitalRead(slic_swHook))    // check if interrupt was from switch hook
      bitSet(sysFlags, sysFlag_offHook);
    else
      bitClear(sysFlags, sysFlag_offHook);
  }

  // perform any pending tasks related to the SLIC
  if  (bitRead(sysFlags, sysFlag_ringing)) {   // if SLIC is ringing the phone, handle the control signals
    slicRingGenerate();
  }

  if (Serial.available()) {           // monitor incoming serial port for commands
    serialChar = Serial.read();
  }

  callProgressGen.update();           // handle mozzi call progress tone gen operations
  runStateMachine();                  // handle state machine operations


} // end loop

// gpio interrupt routine
ICACHE_RAM_ATTR void isr() {
  irq_flag = true;            // set irq active flag so it can be handled in main loop
}

void debug_print_sysflags() {
  Serial.print("Debug sysflags: ");

  if  (bitRead(sysFlags, sysFlag_offHook)) {
    Serial.print("Off Hook | ");
  }
  else {
    Serial.print("On Hook | ");
  }

  if  (bitRead(sysFlags, sysFlag_ringing)) {
    Serial.print("Ringing");
  }
  else {
    Serial.print("Not Ringing");
  }

  Serial.println("\r\n");
}

void runStateMachine() {
  enum states {
    startup,        // initial state
    idle,           // standby mode waiting for activity to occur
    ringTone,       // generate various call progress tones as a demo/test
    ringGenTest,
    dialTone,
    dialToneTest,
    busyTone,
    offHookTone
  };

  static enum states currState = startup;     // initial state is "startup"
  static boolean enteringNewState = true;     // whether or not the current state is just beginning

  switch (currState) {
    case startup:                             // initial state, perform any first-run operations and go to idle state
      Serial.println( F("\r\nStarting up...") );
      currState = idle;
      enteringNewState = true;
      break;

    case idle:
      if (enteringNewState == true) {         // run once when transitioning into state
        Serial.println( F("\r\nEntering Idle state...") );
        Serial.println( F("press n to cycle test tones") );
        Serial.println( F("press r to ring phone") );
        Serial.println( F("take phone off hook to hear dialtone\r\n") );
        enteringNewState = false;
      }

      currState = idle;

      if (serialChar == 'n') {                // transition to next state
        Serial.println( F("Exiting Idle state to generate tones...") );
        debug_print_sysflags();
        serialChar = ' ';                     // clear serial character
        enteringNewState = true;
        currState = dialTone;
      }
      if (serialChar == 'r') {                // transition to phone ring test state
        Serial.println( F("Exiting Idle state to ring phone...") );
        debug_print_sysflags();
        serialChar = ' ';                     // clear serial character
        enteringNewState = true;
        currState = ringGenTest;
      }
      if (offHook()) {                   // if phone goes off hook, play a dial tone
        Serial.println("Phone went off hook. Exiting Idle state");
        debug_print_sysflags();
        digitalWrite(D0, 0);  // debug LED for phone hook
        enteringNewState = true;
        currState = dialToneTest;
      }

      break;

    case ringTone:
      if (enteringNewState == true) {        // run once when transitioning into state
        Serial.println( F("\r\nGenerating ring tone...") );
        callProgressGen.ringToneStart();
        enteringNewState = false;
      }
      if (serialChar == 'n') {               // transition to next state
        Serial.println( F("Stopping ring tone...") );
        serialChar = ' ';                    // clear serial character
        callProgressGen.toneStop();
        enteringNewState = true;
        currState = offHookTone;
      }
      else {
        currState = ringTone;
      }
      break;

    case dialTone:
      if (enteringNewState == true) {        // run once when transitioning into state
        Serial.println( F("\r\nGenerating dial tone...") );
        callProgressGen.dialToneStart();
        enteringNewState = false;
      }
      if (serialChar == 'n') {               // transition to next state
        Serial.println( F("Stopping dial tone...") );
        serialChar = ' ';                    // clear serial character
        callProgressGen.toneStop();
        enteringNewState = true;
        currState = busyTone;
      }
      else {
        currState = dialTone;
      }
      break;

    case dialToneTest:
      if (enteringNewState == true) {        // run once when transitioning into state
        Serial.println( F("\r\nGenerating dial tone...") );
        callProgressGen.dialToneStart();
        enteringNewState = false;
      }
      if (!offHook()) {                 // if phone goes on hook, return to idle state
        Serial.println("\r\nPhone went on hook. Ending dial tone.");
        digitalWrite(D0, 1);  // debug LED for phone hook
        callProgressGen.toneStop();
        debug_print_sysflags();
        enteringNewState = true;
        currState = idle;
      }
      else {
        currState = dialToneTest;
      }
      break;

    case ringGenTest:
      if (enteringNewState == true) {        // run once when transitioning into state
        Serial.println( F("\r\nRinging phone...take phone off hook to return to idle state") );
        ringGenTestStart();
        debug_print_sysflags();
        enteringNewState = false;
      }
      if (!offHook()) {                 // if phone is still on hook, stay in current state
        currState = ringGenTest;
      }
      if (offHook()) {                 // if phone goes off hook, end test and return to idle
        Serial.println( F("Phone is off hook, stopping ring and returning to idle.") );
        slicRingStop();
        debug_print_sysflags();
        currState = idle;
      }
      else {
        currState = ringGenTest;
      }
      break;

    case busyTone:
      if (enteringNewState == true) {         // run once when transitioning into state
        Serial.println( F("\r\nGenerating busy tone...") );
        callProgressGen.busyToneStart();
        enteringNewState = false;
      }
      if (serialChar == 'n') {               // transition to next state
        Serial.println( F("Stopping busy tone...") );
        serialChar = ' ';                    // clear serial character
        callProgressGen.toneStop();
        enteringNewState = true;
        currState = ringTone;
      }
      else {
        currState = busyTone;
      }
      break;

    case offHookTone:
      if (enteringNewState == true) {        // run once when transitioning into state
        Serial.println( F("\r\nGenerating off hook tone...") );
        callProgressGen.offHookToneStart();
        enteringNewState = false;
      }
      if (serialChar == 'n') {               // transition to next state
        Serial.println( F("Stopping off hook tone...") );
        serialChar = ' ';                    // clear serial character
        callProgressGen.toneStop();
        enteringNewState = true;
        currState = idle;
      }
      else {
        currState = offHookTone;
      }
      break;

    default:                                 // in case of undefined state, start over
      currState = startup;
      break;
  }  // end switch case
}  // end state machine

void initSysFlags() {

  if (pcf_gpio.digitalRead(slic_swHook))    // check if phone is on or off hook (check SLIC hook switch)
    bitSet(sysFlags, sysFlag_offHook);
  else
    bitClear(sysFlags, sysFlag_offHook);

  bitClear(sysFlags, sysFlag_ringing);      // phone is not ringing when initializing the system

}

// North America ring cadence  2s on/4s off
void ringNorthAmericaInit() {
  ringCadence[0] = 2000;
  ringCadence[1] = 4000;
  ringCadence[2] = 0;
  ringCadence[3] = 0;
}

// UK ring cadence  0.4s on/0.2s off/0.4s on/2s off
void ringUkInit() {
  ringCadence[0] = 400;
  ringCadence[1] = 200;
  ringCadence[2] = 400;
  ringCadence[3] = 2000;
}

// configure SLIC pins and ring patterns based on region
void initSlic() {

  pcf_gpio.pinMode(slic_fwdRev, OUTPUT);          // configure gpio expander pins
  pcf_gpio.pinMode(slic_ringMode, OUTPUT);
  pcf_gpio.pinMode(slic_swHook, INPUT_PULLUP);

  pcf_gpio.digitalWrite(slic_fwdRev, LOW);
  pcf_gpio.digitalWrite(slic_ringMode, LOW);

  switch (myRegion) {
    case northAmerica:
      ringNorthAmericaInit();
      break;

    case uk:
      ringUkInit();
      break;

    default:
      ringNorthAmericaInit();
      break;
  }
}

// check if the phone is on or off hook
bool offHook() {
  return (bitRead(sysFlags, sysFlag_offHook));
}

// start ringing the phone connected to the SLIC
void ringGenTestStart() {
  if (offHook()) return;                      // don't try to ring a phone unless it is on hook
  slic_ring_cadence_step = 0;                 // start from the beginning of the ring cadence pattern
  slic_timer = millis();                      // start a timer for phone ringing cadence
  slic_ring_freq_timer = millis();            // start a timer for phone ringing freq
  bitSet(sysFlags, sysFlag_ringing);          // phone is ringing, set system status flag
  pcf_gpio.digitalWrite(slic_ringMode, HIGH); // set SLIC in ring generation mode
}

// use SLIC to ring the phone at 20 Hz (25 mS on and 25 mS off)
// with an on/off ring cadence based on selected region
void slicRingGenerate() {
  if (offHook()) {                          // don't ring if phone is off hook
    slicRingStop();
    return;
  }

  static bool fwdRevPinState = LOW;         // use to toggle the SLIC fwd/rev pin while ringing

  // cycle through ring pattern, tracking which cadence step is in progress
  if ((unsigned long)(millis() - slic_timer) > ringCadence[slic_ring_cadence_step]) {
    slic_timer = millis();                               // reset cadence timer for another interval
    slic_ring_cadence_step++;   // go to next part of the ring pattern
    if (slic_ring_cadence_step >= 4) slic_ring_cadence_step = 0; // start at cadence step 0 after the pattern has completed
  }

  // if phone is actively ringing (cadence step 0 or 2)
  // toggle the SLIC fwd/rev pin every half period to generate the target ring frequency
  if ((slic_ring_cadence_step % 2) == 0) {                 // if cadence step # is even eg 0, 2
    if ((unsigned long)(millis() - slic_ring_freq_timer) > (ringFreq / 2)) {
      slic_ring_freq_timer = millis();                     // reset ring freq timer for another interval
      fwdRevPinState = !fwdRevPinState;                    // toggle SLIC pin to generate ring voltage at freq
      pcf_gpio.digitalWrite(slic_fwdRev, fwdRevPinState);
    }
  }
  else pcf_gpio.digitalWrite(slic_fwdRev, LOW);            // if not actively ringing, don't toggle pin

}


void slicRingStop() {
  pcf_gpio.digitalWrite(slic_fwdRev, LOW);   // stop generating ring signal on SLIC
  pcf_gpio.digitalWrite(slic_ringMode, LOW);
  bitClear(sysFlags, sysFlag_ringing);       // phone is not ringing, clear system status flag

  // *** Double check slic documentation regarding the need for a delay for voltages to settle***
}
