//  Serial Debug Monitor
//  Hardware:  Nano on a Nano to Uno breakout board with a TFT touch screen plugged in (old parallel pin model, not SPI)
//  Using MCUFriend TFT library for on screen touch button and scrolling text
//  Scrolling is based on the scroll_kbv  example in the MCUFriend library
//  *** Can't get it working very well so this is just a buggy concept sketch
//      There is a touch button at the top of the screen (portrait mode) that changes the serial data rate from 9600 to 115200
//      to match what is plugged into the Rx and Gnd pins.
//      Serial text is then shown on screen, scrolling up when the screen is full one line at a time
//      Bugs include not properly erasing old text when over-writing a line with new text, and
//      after a full screen has been filled, there is a weird delay and some text moving around on the bottom line before it is
//      updated with the incoming serial data - no idea what is going on


#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

#include <TouchScreen.h>

// run TouchScreen_Calibr_native.ino in MCUFRIEND_kbv examples to calibrate screen
// copy calibration data from serial monitor for XP, XM, YP, YM, TS_LEFT, TS_RT, TS_TOP, TS_BOT
const int XP = 7, XM = A1, YP = A2, YM = 6; //320x480 ID=0x0099
const int TS_LEFT = 903, TS_RT = 163, TS_TOP = 947, TS_BOT = 165;

// touch pressure sensitivity thresholds to detect screen presses
const int MINPRESSURE = 10;
const int MAXPRESSURE = 1000;
int      pixel_x;                                  // touch screen pressed location
int      pixel_y;                                  // touch screen pressed location

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); // create touch screen object

Adafruit_GFX_Button serialButton;                  // create touch screen button for serial rate selection

// define standard colors
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// text/scrolling items
int16_t ht = 16;         // text font pixel height
int16_t top = 3;         // top line to begin text scrolling window
int16_t lines;           // number of text lines on screen
int16_t scroll = 0;      // vertical scroll related - not sure what it does
int16_t width;           // screen pixel width
int     lastYpos = 0;    // track what line the text is on

byte currBpsPos = 0;     // select serial bps rate from this bpsRates array position
const byte numBpsRates = 5;
int bpsRates[numBpsRates] = { 96, 192, 384, 576, 1152 }; // multiply by 100 to get bps rate - keeps data size down to just an int

void setup()
{
  Serial.begin((long)bpsRates[currBpsPos] * 100);       // eg. bpsRates[0] * 100 = 96 * 100 = 9600

  // init display
  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.invertDisplay(true);                              // invert screen colors if this display driver requires it
  tft.setRotation(0);                                   // portrait orientation
  tft.fillScreen(BLACK);

  // draw touch button
  // tft_object, button center X, button center Y, Width, Height, Outline, Text_color, Fill, Button Text, text size
  serialButton.initButton(&tft,  50, 10, 100, 20, WHITE, CYAN, BLACK, "COM Rate", 2);
  serialButton.drawButton(false);

  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);                // system font is 8 pixels.  ht = 8*2=16
  lines = (tft.height() / ht) - 4;   // number of text lines in portrait mode, based on screen and text pixel height and some tweaking

  // print the current serial bps rate at top of screen beside touch button
  tft.setCursor(110, 0);
  tft.print(bpsRates[currBpsPos]);
  tft.print("00  ");

  tft.setCursor(0, tft.height());
  lastYpos = tft.getCursorY();  // store text line position for tracking movement
}

void loop()
{
  // trying to detect when text has started on a new line so scrolling can occur
  // but there is something not working right, and so far there isn't much
  // info about how this works so it's just left sort of working enough for testing
  if (tft.getCursorY() != lastYpos) {
    tft.print("                          ");            // crude way of erasing a line of text before overwriting with new
    tft.setCursor(0, (scroll + top) * ht);
    lastYpos = tft.getCursorY();
    if (++scroll >= lines) scroll = 0;
    tft.vertScroll(top * ht, lines * ht, scroll * ht);
  }

  // when serial data arrives, print it on the display
  if (Serial.available() > 0) {
    char inChar = Serial.read();
    tft.print(inChar);
  }

  // check if the on screen button is pressed and
  // change serial data rate
  bool buttonPressed = Touch_getXY();
  serialButton.press(buttonPressed && serialButton.contains(pixel_x, pixel_y));

  // restart serial port at new bit rate, cycling through bps data rate table
  if (serialButton.justPressed()) {  
    Serial.end();
    currBpsPos++;                                    // switch to next defined serial data rate
    if (currBpsPos >= numBpsRates) currBpsPos = 0;
    Serial.begin((long)bpsRates[currBpsPos] * 100);  // re-init serial port with new data rate

    // update on screen bps rate
    int xpos = tft.getCursorX();                     // save cursor position before moving to top of screen to update serial rate
    int ypos = tft.getCursorY();
    tft.setCursor(110, 0);
    tft.print(bpsRates[currBpsPos]);
    tft.print("00  ");
    tft.setCursor(xpos, ypos);                       // restore cursor position to resume text printing
    delay(50);                                       // debounce touch screen button or it may keep triggering
  }
}

// get touch screen pixel location if screen is touched
bool Touch_getXY(void)
{
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);      
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);   // TFT control pins
  digitalWrite(XM, HIGH);
  bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  if (pressed) {
    pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width());  // map x location from analog pos. reading to screen width
    pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height()); // map y location from analog pos. reading to screen height
  }
  return pressed;
}
