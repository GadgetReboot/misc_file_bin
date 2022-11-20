/*
    Plain Old Telephone Service (POTS) simulator
    former work in progress, now defunct with plans to migrate from ESP8266 to ESP32

    Can generate DTMF call progress tones (selectable regions) using mozzi library
    Uses a PCF8574 GPIO expander to interface with a subscriber line interface circuit module KS0835F SLIC
    When the phone is taken off hook, a dial tone plays
    When the phone is on hook, it can be made to ring using the SLIC module
    When a valid phone number is dialed, the system will attempt to place the call by coordinating with
    another central office (duplicate pcb/phone setup) and if the call is successful, the phones are connected.

    Mozzi documentation/API
		https://sensorium.github.io/Mozzi/doc/html/index.html

   External Libraries Used: Adafruit PCF8574 https://github.com/adafruit/Adafruit_PCF8574
                            Mozzi https://github.com/sensorium/Mozzi

   Tested with Arduino IDE 1.8.13
               ESP8266 board file 3.0.2

   Gadget Reboot
   https://www.youtube.com/gadgetreboot
*/

//#define DEBUG                        // uncomment to allow debug serial messages
#include "DebugUtils.h"
#include "project_globals.h"           // definitions used throughout the project files

// myRegion can be northAmerica or uk (see project_globals.h)
// uncomment the desired region to use:
byte myRegion = northAmerica;
//byte myRegion = uk;

String remotePhoneNumber = "8675309";  // valid phone number for remote phone

#include <Adafruit_PCF8574.h>          // gpio expander library
#include "mozzi_call_progress.h"       // library to generate region specific call progress tones

// create call progress tone generator based on defined region
mozzi_call_progress callProgressGen(myRegion);

Adafruit_PCF8574 pcf_gpio;             // gpio expander object
const byte slic_fwdRev   = 0;          // slic forward/reverse pin on gpio expander
const byte slic_ringMode = 1;          // slic ring mode       pin on gpio expander
const byte slic_swHook   = 2;          // slic switch hook     pin on gpio expander
const byte Q1 = 3;                     // mt8870 dtmf data bits to decode from gpio expander
const byte Q2 = 4;
const byte Q3 = 5;
const byte Q4 = 6;
const byte StD = 7;                    // mt8870 delayed steering = high when valid Q1..Q4 data present

const byte gpioInt = D6;               // gpio expander interrupt input
const byte ringLed = D0;               // led indicating phone is ringing
const byte relayPin = D5;              // relay that connects remote phone line to local phone line
const byte re_dePin = D3;              // RS485 receiver/driver enable
char serialChar = ' ';                 // characters read from serial port will control states
volatile bool irq_flag = false;        // flag to indicate interrupt has occurred
unsigned long slic_timer = millis();   // used for various SLIC timing eg. ringing a phone or off hook time out
unsigned long slic_ring_freq_timer = millis();       // used to generate the proper ring freq
byte slic_ring_cadence_step = 0;                     // which of the cadence intervals is the ring pattern generating?
int ringCadence [] = {0, 0, 0, 0};                   // SLIC phone ring cadence for ring/silence/ring/silence (durations in mS)
const byte ringFreq = 45;                            // generate this freq (Hz) ring voltage (possible issues below 40Hz)
const int ringPeriod = (1 / (float)ringFreq) * 1000; // period of ring frequency in mS

// system status flag register to track the state of various modes and operations
uint8_t sysFlags = 0x00;            // start with all flags 0
const byte sysFlag_offHook  = 0;    // sysFlags bit 0:  0 = phone is on hook       1 = phone is off hook
const byte sysFlag_ringing  = 1;    // sysFlags bit 1:  0 = phone is not ringing   1 = phone is ringing
const byte sysFlag_dtmf     = 2;    // sysFlags bit 2:  0 = no new dtmf data       1 = new dtmf data
const byte sysFlag_StD_trig = 3;    // sysFlags bit 3:  0 = dtmf StD has expired   1 = dtmf StD has not expired

void setup() {
  Serial.begin(115200);
  Serial.swap();  // exchange tx/rx pins to alternate pins for RS485 interface
 
  DEBUG_PRINTLN("\r\nPlain Old Telephone Service simulator");
  DEBUG_PRINT("Region switch: ");

  switch (myRegion) {
    case northAmerica:
      DEBUG_PRINT("North America");
      break;

    case uk:
      DEBUG_PRINT("United Kingdom");
      break;

    default:
      DEBUG_PRINT("North America");
      break;
  }
  DEBUG_PRINTLN();

  callProgressGen.startup();   // initialize mozzi call progress tone generator
  pcf_gpio.begin(0x20, &Wire); // gpio expander address is hard wired to 0x20

  initSysFlags();              // check the system and set status flags
  initSlic();                  // configure SLIC control pins and phone ring cadence based on set region
  initMT8870();                // configure dtmf decoder pins

  // interrupt input for gpio expander input activity monitoring
  pinMode(gpioInt, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(gpioInt), isr, FALLING);

  pinMode(re_dePin, OUTPUT);   // configure RS485 direction pin
  digitalWrite(re_dePin, LOW); // test mode: receive data only

  digitalWrite(ringLed, 1);
  pinMode(ringLed, OUTPUT);    // configure ring indicator led
  digitalWrite(relayPin, LOW);
  pinMode(relayPin, OUTPUT);   // configure relay control pin

} // end setup

void loop() {

  // check if gpio expander interrupt has occurred and perform actions
  // an interrupt is expected when phone goes on or off hook, or when DTMF data available
  if (irq_flag) {
    irq_flag = false;  // clear interrupt flag

    // update phone hook status (detect phone going on or off hook)
    if (pcf_gpio.digitalRead(slic_swHook))
      bitSet(sysFlags, sysFlag_offHook);
    else
      bitClear(sysFlags, sysFlag_offHook);

    // check if dtmf decoder has data to read
    // StD signal remains high while data available
    // set a trigger flag when StD first asserts high
    // ignore further dtmf data until StD has gone low and re-triggers
    if (pcf_gpio.digitalRead(StD)) {
      if (!bitRead(sysFlags, sysFlag_StD_trig)) {
        bitSet(sysFlags, sysFlag_dtmf);
        bitSet(sysFlags, sysFlag_StD_trig);
      }
    }
    else
      bitClear(sysFlags, sysFlag_StD_trig);    // if StD is no longer high, the event is no longer triggered/active
  }

  // perform any pending tasks related to the SLIC
  if  (bitRead(sysFlags, sysFlag_ringing)) {   // if SLIC is ringing the phone, handle the control signals
    slicRingGenerate();
  }

  if (Serial.available()) {                    // monitor incoming serial port for commands
    serialChar = Serial.read();
  }

  callProgressGen.update();                    // handle mozzi call progress tone gen operations
  runStateMachine();                           // handle state machine operations

} // end loop

// gpio interrupt routine
ICACHE_RAM_ATTR void isr() {
  irq_flag = true;                             // set irq active flag so it can be handled in main loop
}

void runStateMachine() {
  enum states {
    startup,              // initial state
    idle,                 // standby mode waiting for activity to occur
    incomingCall,         // a call is being received, ring the phone and wait for a pick up
    connectToRemoteLine,  // connect local phone to remote phone line
    ringGenTest,          // test: make phone ring
    dialToneTest,         // test: generating a dial tone
    ringTone,             // test: generate various call progress tones
    dialTone,
    busyTone,
    offHookTone
  };

  static enum states currState = startup;     // initial state is "startup"
  static boolean enteringNewState = true;     // whether or not the current state is just beginning
  static String dialedNumber = "";            // store dtmf button presses in a string, to be compared against phone numbers

  switch (currState) {
    case startup:                             // initial state, perform any first-run operations and go to idle state
      DEBUG_PRINTLN( F("\r\nStarting up...") );
      currState = idle;
      enteringNewState = true;
      break;

    case idle:
      if (enteringNewState == true) {         // run once when transitioning into state
        DEBUG_PRINTLN( F("\r\nEntering Idle state...") );
        DEBUG_PRINTLN( F("press n to cycle test tones") );
        DEBUG_PRINTLN( F("press r to ring phone") );
        DEBUG_PRINTLN( F("take phone off hook to hear dialtone\r\n") );
        enteringNewState = false;
      }

      currState = idle;

      if (serialChar == 'n') {                // transition to next state
        DEBUG_PRINTLN( F("Exiting Idle state to generate tones...") );
        debug_print_sysflags();
        serialChar = ' ';                     // clear serial character
        enteringNewState = true;
        currState = dialTone;
      }
      if (serialChar == 'r') {                // transition to phone ring test state
        DEBUG_PRINTLN( F("Exiting Idle state to ring phone...") );
        debug_print_sysflags();
        serialChar = ' ';                     // clear serial character
        enteringNewState = true;
        currState = ringGenTest;
      }
      if (serialChar == 'I') {                // incoming call, ring the phone
        DEBUG_PRINTLN( F("Exiting Idle state to ring phone...") );
        serialChar = ' ';                     // clear serial character
        enteringNewState = true;
        currState = incomingCall;
      }
      if (offHook()) {                   // if phone goes off hook, play a dial tone
        DEBUG_PRINTLN("Phone went off hook. Exiting Idle state");
        debug_print_sysflags();
        enteringNewState = true;
        currState = dialToneTest;
      }
      break;

    case ringTone:
      if (enteringNewState == true) {        // run once when transitioning into state
        DEBUG_PRINTLN( F("\r\nGenerating ring tone...") );
        callProgressGen.ringToneStart();
        enteringNewState = false;
      }
      if (serialChar == 'n') {               // transition to next state
        DEBUG_PRINTLN( F("Stopping ring tone...") );
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
        DEBUG_PRINTLN( F("\r\nGenerating dial tone...") );
        callProgressGen.dialToneStart();
        enteringNewState = false;
      }
      if (serialChar == 'n') {               // transition to next state
        DEBUG_PRINTLN( F("Stopping dial tone...") );
        serialChar = ' ';                    // clear serial character
        callProgressGen.toneStop();
        enteringNewState = true;
        currState = busyTone;
      }
      else {
        currState = dialTone;
      }
      break;

    case busyTone:
      if (enteringNewState == true) {         // run once when transitioning into state
        DEBUG_PRINTLN( F("\r\nGenerating busy tone...") );
        callProgressGen.busyToneStart();
        enteringNewState = false;
      }
      if (serialChar == 'n') {               // transition to next state
        DEBUG_PRINTLN( F("Stopping busy tone...") );
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
        DEBUG_PRINTLN( F("\r\nGenerating off hook tone...") );
        callProgressGen.offHookToneStart();
        enteringNewState = false;
      }
      if (serialChar == 'n') {               // transition to next state
        DEBUG_PRINTLN( F("Stopping off hook tone...") );
        serialChar = ' ';                    // clear serial character
        callProgressGen.toneStop();
        enteringNewState = true;
        currState = idle;
      }
      else {
        currState = offHookTone;
      }
      break;

    case dialToneTest:
      if (enteringNewState == true) {        // run once when transitioning into state
        dialedNumber = "";                   // store dtmf button presses in a string
        DEBUG_PRINTLN( F("\r\nGenerating dial tone...") );
        callProgressGen.dialToneStart();
        enteringNewState = false;
      }
      // if dtmf keys are pressed, check for a valid dialed phone number
      if (bitRead(sysFlags, sysFlag_dtmf)) {
        char c_read = readDtmf();
        if (c_read != ' ') {
          callProgressGen.toneStop();                // stop playing dialtone when phone keys get pressed
          dialedNumber = dialedNumber + c_read;      // append dtmf character to dialed number
          DEBUG_PRINTLN(c_read);                     // show the key presses in serial monitor
          if (dialedNumber == remotePhoneNumber) {
            Serial.print('I');                       // tell other node to expect an incoming call
            digitalWrite (relayPin, HIGH);           // turn on relay when a correct number is dialed
          }
        }
      }
      if (!offHook()) {                              // if phone goes on hook, return to idle state
        DEBUG_PRINTLN("\r\nPhone went on hook. Ending dial tone.");
        callProgressGen.toneStop();
        digitalWrite (relayPin, LOW);                // turn off relay when phone is on hook
        debug_print_sysflags();
        enteringNewState = true;
        currState = idle;
      }
      else {
        currState = dialToneTest;
      }
      break;

    case ringGenTest:
      if (enteringNewState == true) {                // run once when transitioning into state
        DEBUG_PRINTLN( F("\r\nRinging phone...take phone off hook to return to idle state") );
        ringGenTestStart();
        debug_print_sysflags();
        enteringNewState = false;
      }
      if (!offHook()) {                              // if phone is still on hook, stay in current state
        currState = ringGenTest;
      }
      if (offHook()) {                               // if phone goes off hook, end test and return to idle
        DEBUG_PRINTLN( F("Phone is off hook, stopping ring and returning to idle.") );
        slicRingStop();
        debug_print_sysflags();
        enteringNewState = true;
        currState = idle;
      }
      else {
        currState = ringGenTest;
      }
      break;

    case connectToRemoteLine:
      if (enteringNewState == true) {                         // run once when transitioning into state
        DEBUG_PRINTLN( F("\r\nConnecting to remote line") );
        digitalWrite (relayPin, HIGH);                        // turn on relay to connect to remote line
        enteringNewState = false;
      }
      if (!offHook()) {                                       // if phone goes on hook, end call
        digitalWrite (relayPin, LOW);                         // turn off relay to disconnect from remote line
        currState = idle;
      }
      else {                                                  // stay connected to remote line as long as phone is off hook
        currState = connectToRemoteLine;
      }
      break;

    case incomingCall:
      if (enteringNewState == true) {                         // run once when transitioning into state
        DEBUG_PRINTLN( F("\r\nIncoming call...") );
        ringGenTestStart();
        debug_print_sysflags();
        enteringNewState = false;
      }
      if (!offHook()) {                                       // if phone is still on hook, stay in current state
        currState = incomingCall;
      }
      if (offHook()) {                                        // if phone goes off hook, connect to other phone
        slicRingStop();
        enteringNewState = true;
        currState = connectToRemoteLine;
      }
      else {
        currState = incomingCall;
      }
      break;

    default:                                                  // in case of undefined state, start over
      currState = startup;
      break;
  }  // end switch case
}  // end state machine
