/*
    functions belonging to main top level .INO sketch file
    offloaded here to keep things uncluttered
*/

#include "miscFunctions.h"
#include "project_globals.h"

// initialize the system status flags to default states
void initSysFlags() {

  if (pcf_gpio.digitalRead(slic_swHook))    // check if phone is on or off hook (check SLIC hook switch)
    bitSet(sysFlags, sysFlag_offHook);
  else
    bitClear(sysFlags, sysFlag_offHook);

  bitClear(sysFlags, sysFlag_ringing);      // phone is not ringing when initializing the system
  bitClear(sysFlags, sysFlag_dtmf);         // assume no dtmf data needs to be decoded
  bitClear(sysFlags, sysFlag_StD_trig);     // assume no dtmf tones are detected on phone line

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

// configure mt8870 pins
void initMT8870() {
  pcf_gpio.pinMode(Q1, INPUT_PULLUP);          // configure gpio expander pins
  pcf_gpio.pinMode(Q2, INPUT_PULLUP);
  pcf_gpio.pinMode(Q3, INPUT_PULLUP);
  pcf_gpio.pinMode(Q4, INPUT_PULLUP);
  pcf_gpio.pinMode(StD, INPUT_PULLUP);
}

void debug_print_sysflags() {
  Serial.print("Debug sysflags: ");

  if  (bitRead(sysFlags, sysFlag_dtmf)) {
    Serial.print("DTMF Tone | ");
  }
  else {
    Serial.print("DTMF None | ");
  }

  if  (bitRead(sysFlags, sysFlag_StD_trig)) {
    Serial.print("DTMF Triggered | ");
  }
  else {
    Serial.print("DTMF !Trig | ");
  }

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

void slicRingStop() {
  pcf_gpio.digitalWrite(slic_fwdRev, LOW);   // stop generating ring signal on SLIC
  pcf_gpio.digitalWrite(slic_ringMode, LOW);
  bitClear(sysFlags, sysFlag_ringing);       // phone is not ringing, clear system status flag

  // *** Double check slic documentation regarding the need for a delay for voltages to settle***
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
      digitalWrite(ringLed, fwdRevPinState);               // toggle ring indicator LED
    }
  }
  else {
    pcf_gpio.digitalWrite(slic_fwdRev, LOW);            // if not actively ringing, don't toggle pin
    digitalWrite(ringLed, HIGH);                        // turn off ring indicator LED
  }

}


// if dtmf data available, read dtmf data
// translate to character key
// debounce StD signal to prevent re-detecting same key press
char readDtmf() {

  if (!bitRead(sysFlags, sysFlag_dtmf)) return ' ';  // if no dtmf data to process, return empty char

  char c = ' ';  // space character means no new data

  uint8_t dtmf = ( 0x00 | (pcf_gpio.digitalRead(Q1) << 0) | (pcf_gpio.digitalRead(Q2) << 1) | (pcf_gpio.digitalRead(Q3) << 2) | (pcf_gpio.digitalRead(Q4) << 3) );
  switch (dtmf)
  {
    case 0x01:
      c = '1';
      break;
    case 0x02:
      c = '2';
      break;
    case 0x03:
      c = '3';
      break;
    case 0x04:
      c = '4';
      break;
    case 0x05:
      c = '5';
      break;
    case 0x06:
      c = '6';
      break;
    case 0x07:
      c = '7';
      break;
    case 0x08:
      c = '8';
      break;
    case 0x09:
      c = '9';
      break;
    case 0x0A:
      c = '0';
      break;
    case 0x0B:
      c = '*';
      break;
    case 0x0C:
      c = '#';
      break;
    default:
      c = ' ';
      break;
  }
  //debug_print_sysflags();
  bitClear(sysFlags, sysFlag_dtmf);  // indicate that dtmf data has been processed

  return c;                          // return the dtmf character received
}
