/*
    Plain Old Telephone Service (POTS) simulator
    Uses 30 pin ESP32 DOIT Devkit V1 module
    Choose Arduino IDE board "DOIT ESP32 DEVKIT V1"

    Uses a subscriber line interface circuit (SLIC) module KS0835F
    Can generate DTMF call progress tones (selectable regions) using mozzi audio library
    Can play back "This is a recording" audio sample stored in program memory using mozzi

    Coordinates with another node wirelessly using ESP-NOW to place or receive calls
    The node configured as a "Server" via a jumper will create a web page interface

    Mozzi documentation/API
		https://sensorium.github.io/Mozzi/doc/html/index.html

   Libraries:   ArduinoJSON
                https://github.com/me-no-dev/ESPAsyncWebServer
                https://github.com/me-no-dev/AsyncTCP
                Mozzi sound library https://github.com/sensorium/Mozzi

   Tested with Arduino IDE 1.8.13
               ESP32 board file 1.0.6

   Gadget Reboot
   https://www.youtube.com/gadgetreboot
*/

//#define DEBUG                        // uncomment to allow debug serial messages
#include "DebugUtils.h"
#include "project_globals.h"           // definitions used throughout the project files
#include "miscFunctions.h"             // some things kept here to de-clutter main .ino
#include "wifiFunctions.h"             // some things kept here to de-clutter main .ino
#include "mozzi_call_progress.h"       // class to generate region specific call progress tones

String remotePhoneNumber = "8675309";  // valid phone number for remote phone

// create call progress tone generator based on defined region
mozzi_call_progress callProgressGen(myRegion);


/*
  void setup() {
  Serial.begin(115200);


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

*/

// gpio interrupt routine
void IRAM_ATTR isr() {
  irq_flag = true;                             // set irq active flag so it can be handled in main loop
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

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

  Serial.println();
  Serial.print("ESP32 MAC Address:  ");
  Serial.println(WiFi.macAddress());

  pinMode(senderServerPin, INPUT);           // pin 35 is input only so there is an external pull up resistor
  wifiMode = digitalRead(senderServerPin);   // set this node as a wifi Sender or Server based on jumper setting

  Serial.println();
  Serial.print("ESP32 Wifi Mode: ");
  Serial.println(wifiMode ? "Sender" : "Server");

  if (wifiMode == wifiServer) initServer();
  else initSender();


  callProgressGen.startup();   // initialize mozzi call progress tone generator

  pcf_gpio.begin(0x20, &Wire); // gpio expander address is hard wired to 0x20

  initSysFlags();              // check the system and set status flags
  initSlic();                  // configure SLIC control pins and phone ring cadence based on set region
  initMT8870();                // configure dtmf decoder pins

  // interrupt input for gpio expander input activity monitoring
  pinMode(gpioInt, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(gpioInt), isr, FALLING);

  digitalWrite(ringLed, 1);
  pinMode(ringLed, OUTPUT);    // configure ring indicator led
  digitalWrite(relayPin, LOW);
  pinMode(relayPin, OUTPUT);   // configure relay control pin

}

void loop() {
  static unsigned long lastEventTime = millis();
  static unsigned long lastEventTime2 = millis();
  static unsigned long lastEventTime3 = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;
  static const unsigned long testInterval = 5000;
  static byte testing = 0;

  callProgressGen.update();                    // handle mozzi call progress tone gen operations

  if ((millis() - lastEventTime3) > testInterval) {
    lastEventTime3 = millis();
    testing++;
  }

  if (testing == 0) {
    testing++;
    callProgressGen.thisIsARecordingStart();
  }

  if (testing == 2) {
    testing++;
    callProgressGen.audioStop();
    callProgressGen.dialToneStart();
  }

  if (testing == 4) {
    testing++;
    callProgressGen.audioStop();
    callProgressGen.ringToneStart();
  }


  if (testing == 6) {
    testing++;
    callProgressGen.audioStop();
    callProgressGen.busyToneStart();
  }

  if (testing == 8) {
    testing++;
    callProgressGen.audioStop();
    callProgressGen.offHookToneStart();
  }

  if (testing == 10) {
    testing = 0;
    callProgressGen.audioStop();
  }


  if (wifiMode == wifiServer) {
    if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
      lastEventTime = millis();
      doServerEvent();
    }
  }
  else if (wifiMode == wifiSender) {
    if (millis() - lastEventTime2 >= EVENT_INTERVAL_MS) {
      // Save the last time a new reading was published
      lastEventTime2 = millis();
      doSenderEvent();
    }
  }
}

/*
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
*/




/*

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
        callProgressGen.audioStop();
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
        callProgressGen.audioStop();
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
        callProgressGen.audioStop();
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
        callProgressGen.audioStop();
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
          callProgressGen.audioStop();                // stop playing dialtone when phone keys get pressed
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
        callProgressGen.audioStop();
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

*/
