/*
  (Arduino) Nano Mega Tester
  Can be used to test cables up to 40 pins at a time

  used Arduino IDE 1.8.13
  Adafruit mcp23017 library 2.0.3
  Adafruit ssd1306 1.2.9  2.0.0
  Adafruit GFX 1.10.0

  Adapted from a project by youtube.com/@AnotherMaker
         Original concept: https://www.youtube.com/watch?v=Qd_1GTdlSL4

  OLED/Rotary Encoder menu system based on project https://github.com/shadicode/Arduino_OLED_UI
  using "MenuSystem" files from https://github.com/jonblack/arduino-menusystem/tree/9b92730bcf48a032541760278b95fdb0a77285fe

  Arduino pin mapping for MCP23017 GPIO expander
  mcp23017 pin    pin name       library pin #
     21            GPA0             0
     22            GPA1             1
     23            GPA2             2
     24            GPA3             3
     25            GPA4             4
     26            GPA5             5
     27            GPA6             6
     28            GPA7             7
      1            GPB0             8
      2            GPB1             9
      3            GPB2             10
      4            GPB3             11
      5            GPB4             12
      6            GPB5             13
      7            GPB6             14
      8            GPB7             15

  youtube.com/@GadgetReboot
*/

#include "MenuSystem.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#include <Adafruit_MCP23X17.h>
Adafruit_MCP23X17 mcp1;  // MCP23017 objects to interface with
Adafruit_MCP23X17 mcp2;
Adafruit_MCP23X17 mcp3;
Adafruit_MCP23X17 mcp4;
Adafruit_MCP23X17 mcp5;

#define addr1 0x20       // mcp23017 addresses (set on pcb with solder jumpers)
#define addr2 0x21
#define addr3 0x22
#define addr4 0x23
#define addr5 0x24

// map mcp23017 pcb pins to Arduino pin numbers
// there are 5 chips mcp1..mcp5, each with Arduino pins 0..15,
// which are assigned as inputs [1..40] or outputs [1..40] across the 5 chips
// it's possible some pins are mapped incorrectly.  if there are unexpected results,
// try verifying against schematic directly in KiCad to be sure it's the latest mapping.

// mcp1 pins
#define output33    0   // GPA0 output
#define output34    1   // GPA1 output
#define output35    2   // GPA2 output
#define output36    3   // GPA3 output
#define output37    4   // GPA4 output
#define output38    5   // GPA5 output
#define output39    6   // GPA6 output
#define output40    7   // GPA7 output

#define output1     8   // GPB0 output
#define output2     9   // GPB1 output
#define output3     10  // GPB2 output
#define output4     11  // GPB3 output
#define output5     12  // GPB4 output
#define output6     13  // GPB5 output
#define output7     14  // GPB6 output
#define output8     15  // GPB7 output

// mcp2 pins
#define output25    0   // GPA0 output
#define output26    1   // GPA1 output
#define output27    2   // GPA2 output
#define output28    3   // GPA3 output
#define output29    4   // GPA4 output
#define output30    5   // GPA5 output
#define output31    6   // GPA6 output
#define output32    7   // GPA7 output

#define output9     8   // GPB0 output
#define output10    9   // GPB1 output
#define output11    10  // GPB2 output
#define output12    11  // GPB3 output
#define output13    12  // GPB4 output
#define output14    13  // GPB5 output
#define output15    14  // GPB6 output
#define output16    15  // GPB7 output

// mcp3 pins
#define input17    0   // GPA0 input
#define input18    1   // GPA1 input
#define input19    2   // GPA2 input
#define input20    3   // GPA3 input
#define input24    4   // GPA4 input
#define input23    5   // GPA5 input
#define input22    6   // GPA6 input
#define input21    7   // GPA7 input

#define output17     8   // GPB0 output
#define output18     9   // GPB1 output
#define output19     10  // GPB2 output
#define output20     11  // GPB3 output
#define output24     12  // GPB4 output
#define output23     13  // GPB5 output
#define output22     14  // GPB6 output
#define output21     15  // GPB7 output

// mcp4 pins
#define input33     0   // GPA0 input
#define input34     1   // GPA1 input
#define input35     2   // GPA2 input
#define input36     3   // GPA3 input
#define input37     4   // GPA4 input
#define input38     5   // GPA5 input
#define input39     6   // GPA6 input
#define input40     7   // GPA7 input

#define input1      8   // GPB0 input
#define input2      9   // GPB1 input
#define input3      10  // GPB2 input
#define input4      11  // GPB3 input
#define input5      12  // GPB4 input
#define input6      13  // GPB5 input
#define input7      14  // GPB6 input
#define input8      15  // GPB7 input

// mcp5 pins
#define input25     0   // GPA0 input
#define input26     1   // GPA1 input
#define input27     2   // GPA2 input
#define input28     3   // GPA3 input
#define input29     4   // GPA4 input
#define input30     5   // GPA5 input
#define input31     6   // GPA6 input
#define input32     7   // GPA7 input

#define input9      8   // GPB0 input
#define input10     9   // GPB1 input
#define input11     10  // GPB2 input
#define input12     11  // GPB3 input
#define input13     12  // GPB4 input
#define input14     13  // GPB5 input
#define input15     14  // GPB6 input
#define input16     15  // GPB7 input

const int displayDelayShort = 500;      // delay while showing misc status text on OLED
const int displayDelayLong = 2000;      // delay while showing misc status text on OLED
const int testDisplayDelay = 10;        // delay while showing test result text on OLED
const int pinFaultDisplayDelay = 500;   // delay while showing list of pin failures
const int debugDelay = 1;               // delay while accessing mcp23017's to test for timing issues

// array elements [0..39] correspond to header test pins [1..40]
// each element [0..39] represents a cable pin on the Output header side
// the number contained in each element represents a pin number
// that a cable pin is mapped to on the Input side of the cable
// testPinMap[0] = 5  means Output pin 1 (array element 0) maps to Input pin 5
// testPinMap[4] = 1  means Output pin 5 (array element 4) maps to Input pin 1
// therefore the cable being tested has a crossover wire connection from pin 1 to 5 and pin 5 to 1
byte testPinMap[40];               // array to map Output pins (testPinMap[xx]) to Input pins (the value of xx)
byte testResultOpen[40];           // array to note which Output pins tested as open circuit
byte testResultShort[40];          // array to note which Output pins tested as shorted or miswired somewhere else
byte testBufIdx = 0;               // index for moving through testResultShort buffer within sketch

// Encoder Pins
const int    PinCLK   = 2;     // Used for generating interrupts using CLK signal
const int    PinDT    = 3;     // Used for reading DT signal
const int    PinSW    = 4;     // Used for the encoder push switch

volatile int virtualPosition = 0;  // track rotary encoder movements for menu selections
int lastCount = 0;

int led = 13;                      // nano built in LED
int ledState = LOW;

// y-coordinate offsets to allow
// 4 lines of text on oled with text size = 1
const byte textLine1 = 0;
const byte textLine2 = 8;
const byte textLine3 = 16;
const byte textLine4 = 24;

// Menu variables
MenuSystem ms;
Menu mm("Rotate for Options");     // root menu text displayed
MenuItem mm_mi1("Test 1/4 TS");    // menu choices available by moving encoder knob
MenuItem mm_mi2("Test RJ45");
MenuItem mm_mi3("Test RJ12");
boolean menuSelected = false;      // no menu item has been selected

void setup()   {

  Serial.begin(115200);
  Serial.println();
  Serial.println("Nano Mega Tester");
  Serial.println("This code is buggy, the Nano crashes when code is modified, and it all stinks bigly.");
  Serial.println();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)

  pinMode(PinCLK, INPUT_PULLUP);
  pinMode(PinDT, INPUT_PULLUP);
  pinMode(PinSW, INPUT_PULLUP);
  pinMode(led, OUTPUT);

  attachInterrupt(0, isr, FALLING);   // interrupt 0 is pin 2

  // oled menu structure, add or remove items
  // to run callback functions
  mm.add_menu(&mm);
  mm.add_item(&mm_mi1, &on_item1_selected);
  mm.add_item(&mm_mi2, &on_item2_selected);
  mm.add_item(&mm_mi3, &on_item3_selected);
  ms.set_root_menu(&mm);

  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK); // Draw white text.  Black background clears previous text instead of overwriting persistently.
  displayMenu();

  // init mcp23017 devices, using the addresses set on board jumpers
  if (!mcp1.begin_I2C(addr1)) {
    Serial.println("Can't initialize mcp23017 U1");
    while (1);
  }
  if (!mcp2.begin_I2C(addr2)) {
    Serial.println("Can't initialize mcp23017 U2");
    while (1);
  }
  if (!mcp3.begin_I2C(addr3)) {
    Serial.println("Can't initialize mcp23017 U3");
    while (1);
  }
  if (!mcp4.begin_I2C(addr4)) {
    Serial.println("Can't initialize mcp23017 U4");
    while (1);
  }
  if (!mcp5.begin_I2C(addr5)) {
    Serial.println("Can't initialize mcp23017 U5");
    while (1);
  }

  // configure gpio expander pins as inputs
  // any output pins will be momentarily set to output mode when actively writing
  // to minimize problems from shorted pins being driven high/low at the same time
  setAllHeadersAsInputs();
}

void loop() {

  if (!(digitalRead(PinSW))) {        // check if pushbutton is pressed
    while (!digitalRead(PinSW)) {}    // and wait for it to release
    ms.select();                      // execute menu item callback function when button is pressed
  }
  checkEncoderRotation();             // see if the encoder has moved and process the display menu
}

// Interrupt service routine is executed when a HIGH to LOW transition is detected on CLK
void isr ()  {

  static unsigned long  lastInterruptTime = 0;
  unsigned long         interruptTime = millis();

  // If interrupts come faster than 5ms, assume it's a bounce and ignore
  if (interruptTime - lastInterruptTime > 5) {
    if (!digitalRead(PinDT))     // determine if encoder rotated clockwise or counter-clockwise and advance menu pos.
      virtualPosition++;
    else
      virtualPosition--;
  }
  lastInterruptTime = interruptTime;
} // ISR



/**************************
    Menu callback functions
 **************************/

void on_item1_selected(MenuItem* p_menu_item) {
  display.clearDisplay();
  display.setCursor(0, 1);
  display.setTextSize(1);
  display.print("Running Test...");
  display.display();
  Serial.println("Running Test...");

  cableTest_TS();  // run test

  displayMenu();    // redraw menu to current selection
}

void on_item2_selected(MenuItem* p_menu_item) {
  display.clearDisplay();
  display.setCursor(0, 1);
  display.setTextSize(1);
  display.print("Running Test...");
  display.display();
  Serial.println("Running Test...");

  cableTest_RJ45();   // run test

  displayMenu();      // redraw menu to current selection
}

void on_item3_selected(MenuItem* p_menu_item) {
  display.clearDisplay();
  display.setCursor(0, 1);
  display.setTextSize(1);
  display.print("Running Test...");
  display.display();
  Serial.println("Running Test...");

  cableTest_RJ12();   // run test

  displayMenu();                // redraw menu to current selection
}

// old test functions to run all 40 pins and to toggle an LED
// when encoder is clicked

/*
  void on_item4_selected(MenuItem* p_menu_item) {
  display.clearDisplay();
  display.setCursor(0, 1);
  display.setTextSize(1);
  display.print("Running Test...");
  display.display();
  Serial.println("Running Test...");

  cableTest40Pins();  // run test

  displayMenu();      // redraw menu to current selection
  }
*/

/*
  void on_item5_selected(MenuItem* p_menu_item) {
  ledState = !ledState;
  digitalWrite(led, ledState);
  display.setCursor(0, 1);
  display.setTextSize(2);

  if (ledState) {
    display.clearDisplay();
    display.print("Led: ON ");
  }
  else {
    display.clearDisplay();
    display.print("Led: OFF ");
  }
  display.display();
  Serial.println("Toggling LED...");
  delay(displayDelayShort);

  displayMenu();      // redraw menu to current selection
  }

*/

/************************************
    Cable Test functions and Examples
 ************************************/

// test all 40 pins, expecting 1:1 wiring
// this demo function isn't currently in the oled menu options
void cableTest40Pins() {

  resetTestResults();  // clear test data arrays before starting a new test
  resetPinMap();       // clear pin mapping before setting up a test

  debugPrintTestResults();  // show how the data arrays are getting configured for debugging
  Serial.println();

  // set up a 40 pin 1:1 cable pin mapping to test, mapping [0..39] to [1..40] array vs pin #
  for (int i = 0; i < 40; i++) {
    testPinMap[i] = i + 1;
  }

  debugPrintTestResults();  // show how the data arrays are getting configured for debugging
  Serial.println();

  // test mapped pins and check for shorts in addition to opens
  Serial.print("Testing pin: ");
  boolean testShorts = true;
  for (int i = 0; i < 40; i++) {
    if (testPinMap[i] != 255) {                        // only test mapped pins
      Serial.print(i + 1); Serial.print(" ");
      testPinContinuity(i, testPinMap[i], testShorts); // test the current pin against the mapped target pin
    }
  }
  Serial.println(); Serial.println();
  debugPrintTestResults();   // show how the data arrays are getting configured for debugging
  Serial.println();
  showTestResults();         // show test results on serial monitor and oled, waiting for user to click encoder button
  Serial.println("Test Complete");
} // end cableTest40Pins


// test a 2 wire Tip/Sleeve mono audio cable in a stereo TRS jack
//     because it's a TRS jack, the 2 conductor cable will detect a connection
//     between ring and sleeve because sleeve is longer and bridges the ring on a mono plug
// Output header side: jack plugged into IO pins 1=Sleeve, 2=Ring(mono Sleeve) 3=Tip
// Input header side: jack plugged into IO pins 35=Sleeve, 36=Ring(mono Sleeve) 37=Tip
void cableTest_TS() {

  resetTestResults();  // clear test data arrays before starting a new test
  resetPinMap();       // clear pin mapping before setting up a test

  debugPrintTestResults();  // show how the data arrays are getting configured for debugging
  Serial.println();

  // set up a 2 pin 1:1 cable pin mapping to test based on where the jacks are plugged in
  testPinMap[0] = 35;   // Sleeve is pin 1 (data array element 0) on Output and pin 35 on Input headers
  testPinMap[1] = 35;   // TRS jack Ring pin will bridge to mono plug Sleeve, so map that as a valid connection
  testPinMap[2] = 37;

  debugPrintTestResults();  // show how the data arrays are getting configured for debugging
  Serial.println();

  // test mapped pins and check for shorts in addition to opens
  Serial.print("Testing pin: ");
  boolean testShorts = true;
  for (int i = 0; i < 40; i++) {
    if (testPinMap[i] != 255) {                         // only test mapped pins
      Serial.print(i + 1); Serial.print(" ");
      testPinContinuity(i, testPinMap[i], !testShorts); // test the current pin against the mapped target pin
    }
  }
  Serial.println(); Serial.println();
  debugPrintTestResults();   // show how the data arrays are getting configured for debugging
  Serial.println();
  showTestResults();         // show test results on serial monitor and oled, waiting for user to click encoder button
  Serial.println("Test Complete");
}

// test a 2 wire Tip/Ring RJ12 phone cable
// Output header side: jack plugged into IO pins 3, 4
// Input header side: jack plugged into IO pins 37, 38
void cableTest_RJ12() {

  resetTestResults();  // clear test data arrays before starting a new test
  resetPinMap();       // clear pin mapping before setting up a test

  debugPrintTestResults();  // show how the data arrays are getting configured for debugging
  Serial.println();

  // set up a 2 pin 1:1 cable pin mapping to test based on where the jacks are plugged in
  testPinMap[2] = 37;
  testPinMap[3] = 38;

  debugPrintTestResults();  // show how the data arrays are getting configured for debugging
  Serial.println();

  // test mapped pins and check for shorts in addition to opens
  Serial.print("Testing pin: ");
  boolean testShorts = true;
  for (int i = 0; i < 40; i++) {
    if (testPinMap[i] != 255) {                        // only test mapped pins
      Serial.print(i + 1); Serial.print(" ");
      testPinContinuity(i, testPinMap[i], testShorts); // test the current pin against the mapped target pin
    }
  }
  Serial.println(); Serial.println();
  debugPrintTestResults();   // show how the data arrays are getting configured for debugging
  Serial.println();
  showTestResults();         // show test results on serial monitor and oled, waiting for user to click encoder button
  Serial.println("Test Complete");
}

// test an 8 pin RJ45 cable
// Output header side: jack plugged into IO pins 1 to 8
// Input header side: jack plugged into IO pins 32 to 39
void cableTest_RJ45() {

  resetTestResults();  // clear test data arrays before starting a new test
  resetPinMap();       // clear pin mapping before setting up a test

  debugPrintTestResults();  // show how the data arrays are getting configured for debugging
  Serial.println();

  // set up an 8 pin 1:1 cable pin mapping to test based on where the jacks are plugged in
  testPinMap[0] = 32;
  testPinMap[1] = 33;
  testPinMap[2] = 34;
  testPinMap[3] = 35;
  testPinMap[4] = 36;
  testPinMap[5] = 37;
  testPinMap[6] = 38;
  testPinMap[7] = 39;

  debugPrintTestResults();  // show how the data arrays are getting configured for debugging
  Serial.println();

  // test mapped pins and check for shorts in addition to opens
  Serial.print("Testing pin: ");
  boolean testShorts = true;
  for (int i = 0; i < 40; i++) {
    if (testPinMap[i] != 255) {                        // only test mapped pins
      Serial.print(i + 1); Serial.print(" ");
      testPinContinuity(i, testPinMap[i], testShorts); // test the current pin against the mapped target pin
    }
  }
  Serial.println(); Serial.println();
  debugPrintTestResults();   // show how the data arrays are getting configured for debugging
  Serial.println();
  showTestResults();         // show test results on serial monitor and oled, waiting for user to click encoder button
  Serial.println("Test Complete");
}

/******************************
       Pin Test Functions
 ******************************/

// test for continuity between 2 mapped pins
// optionally test if each pin is shorted to any other mapped cable pins

void testPinContinuity(byte firstPin, byte secondPin, boolean testShorts) {

  // pins to test, constrained within 1 to 40
  byte sourcePin = constrain((firstPin + 1), 1, 40); // +1 offset added to let [0.39] array match cable [1..40] pin #
  byte destPin = constrain((secondPin), 1, 40);

  delay(debugDelay);        // tweaking while debugging, isn't apparently needed
  setAllHeadersAsInputs();  // make sure no pins are set as outputs except the one about to be tested

  // assert output test signal (logic low) on output header pin to test
  switch (sourcePin) {
    case 1:
      mcp1.pinMode(output1, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output1, LOW);      // set output low
      break;
    case 2:
      mcp1.pinMode(output2, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output2, LOW);      // set output low
      break;
    case 3:
      mcp1.pinMode(output3, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output3, LOW);      // set output low
      break;
    case 4:
      mcp1.pinMode(output4, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output4, LOW);      // set output low
      break;
    case 5:
      mcp1.pinMode(output5, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output5, LOW);      // set output low
      break;
    case 6:
      mcp1.pinMode(output6, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output6, LOW);      // set output low
      break;
    case 7:
      mcp1.pinMode(output7, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output7, LOW);      // set output low
      break;
    case 8:
      mcp1.pinMode(output8, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output8, LOW);      // set output low
      break;
    case 9:
      mcp2.pinMode(output9, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output9, LOW);      // set output low
      break;
    case 10:
      mcp2.pinMode(output10, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output10, LOW);      // set output low
      break;
    case 11:
      mcp2.pinMode(output11, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output11, LOW);      // set output low
      break;
    case 12:
      mcp2.pinMode(output12, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output12, LOW);      // set output low
      break;
    case 13:
      mcp2.pinMode(output13, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output13, LOW);      // set output low
      break;
    case 14:
      mcp2.pinMode(output14, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output14, LOW);      // set output low
      break;
    case 15:
      mcp2.pinMode(output15, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output15, LOW);      // set output low
      break;
    case 16:
      mcp2.pinMode(output16, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output16, LOW);      // set output low
      break;
    case 17:
      mcp3.pinMode(output17, OUTPUT);        // set test pin as output
      mcp3.digitalWrite(output7, LOW);      // set output low
      break;
    case 18:
      mcp3.pinMode(output18, OUTPUT);        // set test pin as output
      mcp3.digitalWrite(output18, LOW);      // set output low
      break;
    case 19:
      mcp3.pinMode(output19, OUTPUT);        // set test pin as output
      mcp3.digitalWrite(output19, LOW);      // set output low
      break;
    case 20:
      mcp3.pinMode(output20, OUTPUT);        // set test pin as output
      mcp3.digitalWrite(output20, LOW);      // set output low
      break;
    case 21:
      mcp3.pinMode(output21, OUTPUT);        // set test pin as output
      mcp3.digitalWrite(output21, LOW);      // set output low
      break;
    case 22:
      mcp3.pinMode(output22, OUTPUT);        // set test pin as output
      mcp3.digitalWrite(output22, LOW);      // set output low
      break;
    case 23:
      mcp3.pinMode(output23, OUTPUT);        // set test pin as output
      mcp3.digitalWrite(output23, LOW);      // set output low
      break;
    case 24:
      mcp3.pinMode(output24, OUTPUT);        // set test pin as output
      mcp3.digitalWrite(output24, LOW);      // set output low
      break;
    case 25:
      mcp2.pinMode(output25, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output25, LOW);      // set output low
      break;
    case 26:
      mcp2.pinMode(output26, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output26, LOW);      // set output low
      break;
    case 27:
      mcp2.pinMode(output27, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output27, LOW);      // set output low
      break;
    case 28:
      mcp2.pinMode(output28, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output28, LOW);      // set output low
      break;
    case 29:
      mcp2.pinMode(output29, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output29, LOW);      // set output low
      break;
    case 30:
      mcp2.pinMode(output30, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output30, LOW);      // set output low
      break;
    case 31:
      mcp2.pinMode(output31, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output31, LOW);      // set output low
      break;
    case 32:
      mcp2.pinMode(output32, OUTPUT);        // set test pin as output
      mcp2.digitalWrite(output32, LOW);      // set output low
      break;
    case 33:
      mcp1.pinMode(output33, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output33, LOW);      // set output low
      break;
    case 34:
      mcp1.pinMode(output34, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output34, LOW);      // set output low
      break;
    case 35:
      mcp1.pinMode(output35, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output35, LOW);      // set output low
      break;
    case 36:
      mcp1.pinMode(output36, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output36, LOW);      // set output low
      break;
    case 37:
      mcp1.pinMode(output37, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output37, LOW);      // set output low
      break;
    case 38:
      mcp1.pinMode(output38, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output38, LOW);      // set output low
      break;
    case 39:
      mcp1.pinMode(output39, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output39, LOW);      // set output low
      break;
    case 40:
      mcp1.pinMode(output40, OUTPUT);        // set test pin as output
      mcp1.digitalWrite(output40, LOW);      // set output low
      break;
    default:
      break;
  } // end switch sourcePin

  // with the one test output pin asserted logic low, now check the
  // second (mapped) test pin to see if it is receiving the asserted signal
  boolean result = checkPinAsserted(destPin);
  if (!result) {
    testResultOpen[sourcePin - 1] = sourcePin; // log this pin as an open pin failure
  }
  else testResultOpen[sourcePin - 1] = 0;       // log this pin as successful test

  // to check for possible short circuits between pins or miswired pins (other pins being asserted unexpectedly),
  // check all input pins to see if any other than the expected pin (destPin)
  // are asserted, indicating a wire connection error or shorted pin
  if (testShorts) {
    for (byte i = 0; i < 40; i++) {                 // cycle through all 40 pins
      boolean checkPin = checkPinAsserted(i + 1);   // check if pin is asserted
      if (checkPin && (i + 1) != destPin ) {        // test for shorts, ignoring the pin it's supposed to be mapped to
        testResultShort[testBufIdx] = sourcePin;    // log pin as short/miswiring failure
        testBufIdx++;
      }
    }
  }
} // end void testPinContinuity

// test a specific pin on the input header to see if it is receiving an
// asserted test signal (logic low) from the output header
// returns true if the pin is seeing the asserted logic low test signal
boolean checkPinAsserted(byte pin) {

  byte testPin = constrain(pin, 1, 40);   // pin to test, constrained within 1 to 40 range
  boolean result = true;                  // test result status
  delay(debugDelay);                      // tweaking while debugging, not needed

  // read the specified input pin to test cable connection, logic high=fail low=pass
  // the inputs have a pull up resistor so to test that the pin is receiving a
  // test signal (asserted low), a logic low needs to be read on the pin
  // to indicate it is receiving the test signal from the output header.
  // this can be used to detect an intentional cable wiring connection as well
  // as an unexpected short circuit being seen on the pin when it wasn't supposed to be asserted

  switch (testPin) {
    case 1:
      if (mcp4.digitalRead(input1)) result = false;      // the switch case is set up to directly read the
      break;                                             // input pin (1 thru 40) on the specific pin (0 thru 15)
    case 2:                                              // of the specific mcp gpio chip (1 thru 5) as wired
      if (mcp4.digitalRead(input2)) result = false;      // on the pcb 40 pin headers
      break;
    case 3:
      if (mcp4.digitalRead(input3)) result = false;
      break;
    case 4:
      if (mcp4.digitalRead(input4)) result = false;
      break;
    case 5:
      if (mcp4.digitalRead(input5)) result = false;
      break;
    case 6:
      if (mcp4.digitalRead(input6)) result = false;
      break;
    case 7:
      if (mcp4.digitalRead(input7)) result = false;
      break;
    case 8:
      if (mcp4.digitalRead(input8)) result = false;
      break;
    case 9:
      if (mcp5.digitalRead(input9)) result = false;
      break;
    case 10:
      if (mcp5.digitalRead(input10)) result = false;
      break;
    case 11:
      if (mcp5.digitalRead(input11)) result = false;
      break;
    case 12:
      if (mcp5.digitalRead(input12)) result = false;
      break;
    case 13:
      if (mcp5.digitalRead(input13)) result = false;
      break;
    case 14:
      if (mcp5.digitalRead(input14)) result = false;
      break;
    case 15:
      if (mcp5.digitalRead(input15)) result = false;
      break;
    case 16:
      if (mcp5.digitalRead(input16)) result = false;
      break;
    case 17:
      if (mcp3.digitalRead(input17)) result = false;
      break;
    case 18:
      if (mcp3.digitalRead(input18)) result = false;
      break;
    case 19:
      if (mcp3.digitalRead(input19)) result = false;
      break;
    case 20:
      if (mcp3.digitalRead(input20)) result = false;
      break;
    case 21:
      if (mcp3.digitalRead(input21)) result = false;
      break;
    case 22:
      if (mcp3.digitalRead(input22)) result = false;
      break;
    case 23:
      if (mcp3.digitalRead(input23)) result = false;
      break;
    case 24:
      if (mcp3.digitalRead(input24)) result = false;
      break;
    case 25:
      if (mcp5.digitalRead(input25)) result = false;
      break;
    case 26:
      if (mcp5.digitalRead(input26)) result = false;
      break;
    case 27:
      if (mcp5.digitalRead(input27)) result = false;
      break;
    case 28:
      if (mcp5.digitalRead(input28)) result = false;
      break;
    case 29:
      if (mcp5.digitalRead(input29)) result = false;
      break;
    case 30:
      if (mcp5.digitalRead(input30)) result = false;
      break;
    case 31:
      if (mcp5.digitalRead(input31)) result = false;
      break;
    case 32:
      if (mcp5.digitalRead(input32)) result = false;
      break;
    case 33:
      if (mcp4.digitalRead(input33)) result = false;
      break;
    case 34:
      if (mcp4.digitalRead(input34)) result = false;
      break;
    case 35:
      if (mcp4.digitalRead(input35)) result = false;
      break;
    case 36:
      if (mcp4.digitalRead(input36)) result = false;
      break;
    case 37:
      if (mcp4.digitalRead(input37)) result = false;
      break;
    case 38:
      if (mcp4.digitalRead(input38)) result = false;
      break;
    case 39:
      if (mcp4.digitalRead(input39)) result = false;
      break;
    case 40:
      if (mcp4.digitalRead(input40)) result = false;
      break;
    default:
      break;
  }
  return result;
} // end checkPinAsserted

/***************************
     Utility functions
****************************/

// show menu choices on oled
void displayMenu() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);

  // Display the menu
  Menu const* cp_menu = ms.get_current_menu();
  display.println(cp_menu->get_selected()->get_name());
  display.display();
}

// debug printout of test array data
// 0 means no failure, 1..40 means a pin with a failure, 255 means pin not being tested
void debugPrintTestResults() {
  Serial.print("Test Pin Map Array Data: ");
  for (int i = 0; i < 40; i++) {
    Serial.print(testPinMap[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Open Test Pin Array Data: ");
  for (int i = 0; i < 40; i++) {
    Serial.print(testResultOpen[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Short Test Pin Array Data: ");
  for (int i = 0; i < 40; i++) {
    Serial.print(testResultShort[i]);
    Serial.print(" ");
  }
  Serial.println();
}

// set all GPIO expander pins as inputs
// by default they should idle as inputs and only
// have one at a time set as an output to prevent
// two different active signals driving each other
void setAllHeadersAsInputs() {
  for (int i = 0; i < 16; i++) {
    delay(debugDelay);                // tweaking while debugging, not needed
    mcp1.pinMode(i, INPUT_PULLUP);
    delay(debugDelay);                // tweaking while debugging, not needed
    mcp2.pinMode(i, INPUT_PULLUP);
    delay(debugDelay);                // tweaking while debugging, not needed
    mcp3.pinMode(i, INPUT_PULLUP);
    delay(debugDelay);                // tweaking while debugging, not needed
    mcp4.pinMode(i, INPUT_PULLUP);
    delay(debugDelay);                // tweaking while debugging, not needed
    mcp5.pinMode(i, INPUT_PULLUP);
  }
}

// see if the rotary encoder has moved and if so
// update the oled menu options
void checkEncoderRotation() {
  if (virtualPosition != lastCount) {
    if (lastCount < virtualPosition)
    {
      ms.next();
      displayMenu();
      display.display();
    }
    else if (lastCount > virtualPosition)
    {
      ms.prev();
      displayMenu();
      display.display();
    }
    lastCount = virtualPosition;
  }
}

// clear test result array data before starting a new test
void resetTestResults() {
  for (int i = 0; i < 40; i++) {
    testBufIdx = 0;                // start at beginning of test data buffer when starting new tests
    testResultOpen[i] = 255;       // 255 means pin wasn't tested so there is no result to report
    testResultShort[i] = 255;
  }
}

// clear test pin mapping before setting up a new test
void resetPinMap() {
  for (int i = 0; i < 40; i++) {
    testPinMap[i] = 255;         // 255 means pin isn't mapped and won't be tested
  }
}

// print a text string on oled at specified coordinates
/* This started crashing for some reason after it had been working.
  Maybe due to running out of program or memory space?
  Resorting to manually pasting the function code where needing to
  call the function is the workaround*/
/*
  void displayString(String text, int x, int y) {
  display.setCursor(x, y);
  display.setTextSize(1);
  display.print(text);
  display.display();
  }
*/

// print test results on oled and serial monitor
// showing pins with detected open or short circuits
void showTestResults() {

  boolean testPassOpen = true;   // test result is true if no opens were detected
  boolean testPassShort = true;  // test result is true if no shorts were detected

  // show test results on serial monitor and
  // check if there are any failures to report
  Serial.print("Pins not connected as expected in pin mapping: ");
  for (int i = 0; i < 40; i++) {
    if ((testResultOpen[i] != 0) && (testResultOpen[i] != 255)) {
      testPassOpen = false;  // indicate test has failed
      Serial.print(testResultOpen[i]);
      Serial.print(" ");
      String s = "Fail Open:" + String(i) + "        ";
      // display message on oled
      display.setCursor(0, textLine4);
      display.setTextSize(1);
      display.print(s);
      display.display();
    }
  }
  if (testPassOpen) Serial.print("None!");
  Serial.println();

  Serial.print("Pins detected as miswired or shorting to other pins: ");
  for (int i = 0; i < 40; i++) {
    if ((testResultShort[i] != 0) && (testResultShort[i] != 255)) {
      testPassShort = false;  // indicate test has failed
      Serial.print(testResultShort[i]);
      Serial.print(" ");
      String s = "Fail Short:" + String(i) + "        ";
      // display message on oled
      display.setCursor(0, textLine4);
      display.setTextSize(1);
      display.print(s);
      display.display();

    }
  }
  if (testPassShort) Serial.print("None!");
  Serial.println();

  // show test fail details on oled until encoder button is pressed
  if ((!testPassOpen) | (!testPassShort)) {                // if there were any failures at all, display them
    unsigned long displayTimer = millis();                 // timer for cycling through error display text
    boolean buttonClicked = false;                         // detect when encoder button is clicked
    boolean showNextFault = true;                          // flag for cycling displayed faults based on timer
    byte failTypeIdx = 0;                                  // for error reporting 0=Opens 1=Shorts
    testBufIdx = 0;                                        // reset index pointer for scanning through test buffers

    // display message on oled
    display.setCursor(0, textLine1);
    display.setTextSize(1);
    display.print("Click Encoder To Exit");
    display.display();


    while (!buttonClicked) {
      // check if encoder button is pressed
      if (!(digitalRead(PinSW))) {        // check if pushbutton is pressed
        while (!digitalRead(PinSW)) {}    // and wait for it to release
        buttonClicked = true;             // track encoder being clicked
        break;                            // stop showing test results after button is released
      }

      // after display timer expires, show the next failure
      if ((millis() - displayTimer) > pinFaultDisplayDelay) {
        displayTimer = millis();
        showNextFault = true;
      }

      // show the next fault when the display timer expires
      if (showNextFault) {
        if (failTypeIdx == 0) {           // if reporting open failures, check for them and show them
          for (int i = testBufIdx; i < 40; i++) {
            if ((testResultOpen[i] != 0) && (testResultOpen[i] != 255)) {    // if this pin has an open fault
              showNextFault = false;            // if a fault is found, wait for display timeout before showing next one
              display.setCursor(0, textLine3);
              display.setTextSize(1);
              display.print("Fail Open ");
              display.print(i + 1);             // show the failed IO pin number
              display.print("   ");
              display.display();
              delay(displayDelayShort);
              break;  // stop checking for errors if one is found on this open fault report cycle
            }
          }
          testBufIdx++;                           // increment to next test buffer item
          if (testBufIdx == 40) {
            testBufIdx = 0;                       // reset index when all open faults are reported
            showNextFault = true;                 // all open pins were reported, show short faults or re-start opens


            /*
                add this line back in if the next commented out section gets put back in
              if (!testPassShort) failTypeIdx = 1;  // switch to reporting short failures if there are any
            */

          }
        } // end if failTypeIdx==0


        /*
             This causes weird crashes on the nano so it's going to be used in a different board

                if (failTypeIdx == 1) {           // if reporting short failures, check for them and show them
                  for (int i = testBufIdx; i < 40; i++) {
                    if ((testResultShort[i] != 0) && (testResultShort[i] != 255)) {    // if this pin has a short fault
                      showNextFault = false;            // if a fault is found, wait for display timeout before showing next one
                      display.setCursor(0, textLine3);
                      display.setTextSize(1);
                      display.print("Fail Short ");
                      display.print(i + 1);             // show the failed IO pin number
                      display.print("   ");
                      display.display();
                      delay(displayDelayShort);
                      break;  // stop checking for errors if one is found on this short fault report cycle
                    }
                  }
                  testBufIdx++;                           // increment to next test buffer item
                  if (testBufIdx == 40) {
                    testBufIdx = 0;                       // reset index when all open faults are reported
                    showNextFault = true;                 // all open pins were reported, show short faults or re-start opens
                    Serial.println("Breakpoint testbuf=40");
                    if (!testPassOpen) failTypeIdx = 0;  // switch to reporting open failures if there are any
                  }
                } // end if failTypeIdx==1
        */

      } // end if showNextFault
    } // end while !buttonClicked
  } else // show test pass message if there are no failures detected
  {
    Serial.println("Test Pass");
    display.setCursor(0, textLine3);
    display.setTextSize(1);
    display.print("Test Pass");
    display.display();
    delay(displayDelayLong);
  }
  // end if !testPassOpen or Short
}  // end showTestResults
