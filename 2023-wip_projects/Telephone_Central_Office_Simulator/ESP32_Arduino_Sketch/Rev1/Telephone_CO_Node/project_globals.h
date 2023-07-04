
#ifndef project_globals_h
#define project_globals_h

// region definitions for different call progress tones
#define northAmerica 0
#define uk           1
#include <Adafruit_PCF8574.h>          // gpio expander library

extern byte myRegion;
extern Adafruit_PCF8574 pcf_gpio;             // gpio expander object
extern const byte slic_fwdRev;          // slic forward/reverse pin on gpio expander
extern const byte slic_ringMode;          // slic ring mode       pin on gpio expander
extern const byte slic_swHook;          // slic switch hook     pin on gpio expander
extern const byte Q1;                     // mt8870 dtmf data bits to decode from gpio expander
extern const byte Q2;
extern const byte Q3;
extern const byte Q4;
extern const byte StD;                    // mt8870 delayed steering = high when valid Q1..Q4 data present

extern const byte senderServerPin;   // gpio35 low = Server  high = Sender  mode for wifi purposes

extern uint8_t sysFlags;            // start with all flags 0
extern const byte sysFlag_offHook;    // sysFlags bit 0:  0 = phone is on hook       1 = phone is off hook
extern const byte sysFlag_ringing;    // sysFlags bit 1:  0 = phone is not ringing   1 = phone is ringing
extern const byte sysFlag_dtmf;    // sysFlags bit 2:  0 = no new dtmf data       1 = new dtmf data
extern const byte sysFlag_StD_trig;    // sysFlags bit 3:  0 = dtmf StD has expired   1 = dtmf StD has not expired

extern const byte gpioInt;               // gpio expander interrupt input
extern const byte ringLed;               // led indicating phone is ringing
extern const byte relayPin;              // relay that connects remote phone line to local phone line
extern const byte re_dePin;              // RS485 receiver/driver enable

extern char serialChar;                 // characters read from serial port will control states
extern volatile bool irq_flag;        // flag to indicate interrupt has occurred
extern unsigned long slic_timer;   // used for various SLIC timing eg. ringing a phone or off hook time out
extern unsigned long slic_ring_freq_timer;       // used to generate the proper ring freq
extern byte slic_ring_cadence_step;                     // which of the cadence intervals is the ring pattern generating?
extern int ringCadence [];                   // SLIC phone ring cadence for ring/silence/ring/silence (durations in mS)
extern const byte ringFreq;                            // generate this freq (Hz) ring voltage (possible issues below 40Hz)
extern const int ringPeriod; // period of ring frequency in mS


#endif
