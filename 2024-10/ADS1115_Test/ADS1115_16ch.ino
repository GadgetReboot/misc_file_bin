/*
 ADS1115 16 ch analog reading test
 Uses Arduino Uno and four ADS1115 modules 
 Each module has its address tied to one of 0v, VCC, SCL, SDA to allow four ADCs on a single I2C bus

  used Arduino IDE 1.8.13
  Adafruit ADS1X15 library 1.0.1
  
  youtube.com/@GadgetReboot
*/

#include <Wire.h>
#include <Adafruit_ADS1015.h>

// ads1115 addresses:
// addr pin = 0v:  0x48
// addr pin = vcc: 0x49
// addr pin = SDA: 0x4A
// addr pin = SCL: 0x4B

Adafruit_ADS1115 ads0(0x48);  // ADC object at I2C address 0x48 for addr pin = GND
Adafruit_ADS1115 ads1(0x49);  // ADC object at I2C address 0x49 for addr pin = 5V
Adafruit_ADS1115 ads2(0x4A);  // ADC object at I2C address 0x4A for addr pin = SDA
Adafruit_ADS1115 ads3(0x4B);  // ADC object at I2C address 0x4B for addr pin = SCL

int16_t adc0, adc1, adc2, adc3;      // variables to hold ADC readings for 1st ads1115 device
int16_t adc4, adc5, adc6, adc7;      // variables to hold ADC readings for 2nd ads1115 device
int16_t adc8, adc9, adc10, adc11;    // variables to hold ADC readings for 3rd ads1115 device
int16_t adc12, adc13, adc14, adc15;  // variables to hold ADC readings for 4th ads1115 device

float multiplier = 0.1875F;               // ADS1115  @ +/- 6.144V gain = 0.1875mV/step

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Read 16 analog inputs on four ADS1115 devices");
  Serial.println();

  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1115
  //                                                                -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.0078125mV

  // init ADS1115 ADC
  ads0.begin();
  ads1.begin();
  ads2.begin();
  ads3.begin();
}

void loop(void) {

  // read in analog inputs on 1st ads1115
  adc0 = ads0.readADC_SingleEnded(0);
  adc1 = ads0.readADC_SingleEnded(1);
  adc2 = ads0.readADC_SingleEnded(2);
  adc3 = ads0.readADC_SingleEnded(3);

  // read in analog inputs on 2nd ads1115
  adc4 = ads1.readADC_SingleEnded(0);
  adc5 = ads1.readADC_SingleEnded(1);
  adc6 = ads1.readADC_SingleEnded(2);
  adc7 = ads1.readADC_SingleEnded(3);

  // read in analog inputs on 3rd ads1115
  adc8 = ads2.readADC_SingleEnded(0);
  adc9 = ads2.readADC_SingleEnded(1);
  adc10 = ads2.readADC_SingleEnded(2);
  adc11 = ads2.readADC_SingleEnded(3);

  // read in analog inputs on 4th ads1115
  adc12 = ads3.readADC_SingleEnded(0);
  adc13 = ads3.readADC_SingleEnded(1);
  adc14 = ads3.readADC_SingleEnded(2);
  adc15 = ads3.readADC_SingleEnded(3);

  Serial.print("A0: "); Serial.print(adc0 * multiplier); Serial.print("mV  ");
  Serial.print("A1: ");  Serial.print(adc1 * multiplier); Serial.print("mV  ");
  Serial.print("A2: ");  Serial.print(adc2 * multiplier); Serial.print("mV  ");
  Serial.print("A3: ");  Serial.print(adc3 * multiplier); Serial.print("mV  ");

  Serial.println();

  Serial.print("A4: ");  Serial.print(adc4 * multiplier); Serial.print("mV  ");
  Serial.print("A5: ");  Serial.print(adc5 * multiplier); Serial.print("mV  ");
  Serial.print("A6: ");  Serial.print(adc6 * multiplier); Serial.print("mV  ");
  Serial.print("A7: ");  Serial.print(adc7 * multiplier); Serial.print("mV  ");

  Serial.println();

  Serial.print("A8: ");  Serial.print(adc8 * multiplier); Serial.print("mV  ");
  Serial.print("A9: ");  Serial.print(adc9 * multiplier); Serial.print("mV  ");
  Serial.print("A10: ");  Serial.print(adc10 * multiplier); Serial.print("mV  ");
  Serial.print("A11: ");  Serial.print(adc11 * multiplier); Serial.print("mV  ");

  Serial.println();

  Serial.print("A12: ");  Serial.print(adc12 * multiplier); Serial.print("mV  ");
  Serial.print("A13: ");  Serial.print(adc13 * multiplier); Serial.print("mV  ");
  Serial.print("A14: ");  Serial.print(adc14 * multiplier); Serial.print("mV  ");
  Serial.print("A15: ");  Serial.print(adc15 * multiplier); Serial.print("mV  ");

  Serial.println(); Serial.println();

  //see how long ADC readings take
  unsigned long startTime;
  unsigned long stopTime;

  // read single analog input
  startTime = micros();
  adc0 = ads0.readADC_SingleEnded(0);
  stopTime = micros();
  Serial.print("Single A0 input reading took "); Serial.print(stopTime - startTime); Serial.print(" uS");
  Serial.println();  

  // read 4 analog inputs
  startTime = micros();
  adc0 = ads0.readADC_SingleEnded(0);
  adc1 = ads0.readADC_SingleEnded(1);
  adc2 = ads0.readADC_SingleEnded(2);
  adc3 = ads0.readADC_SingleEnded(3);
  stopTime = micros();
  Serial.print("A0 to A3 input readings took "); Serial.print(stopTime - startTime); Serial.print(" uS");
  Serial.println();  

  // read 8 analog inputs
  startTime = micros();
  adc0 = ads0.readADC_SingleEnded(0);
  adc1 = ads0.readADC_SingleEnded(1);
  adc2 = ads0.readADC_SingleEnded(2);
  adc3 = ads0.readADC_SingleEnded(3);
  adc4 = ads1.readADC_SingleEnded(0);
  adc5 = ads1.readADC_SingleEnded(1);
  adc6 = ads1.readADC_SingleEnded(2);
  adc7 = ads1.readADC_SingleEnded(3);
  stopTime = micros();
  Serial.print("A0 to A7 input readings took "); Serial.print(stopTime - startTime); Serial.print(" uS");
  Serial.println();  

  // read 12 analog inputs
  startTime = micros();
  adc0 = ads0.readADC_SingleEnded(0);
  adc1 = ads0.readADC_SingleEnded(1);
  adc2 = ads0.readADC_SingleEnded(2);
  adc3 = ads0.readADC_SingleEnded(3);
  adc4 = ads1.readADC_SingleEnded(0);
  adc5 = ads1.readADC_SingleEnded(1);
  adc6 = ads1.readADC_SingleEnded(2);
  adc7 = ads1.readADC_SingleEnded(3);
  adc8 = ads2.readADC_SingleEnded(0);
  adc9 = ads2.readADC_SingleEnded(1);
  adc10 = ads2.readADC_SingleEnded(2);
  adc11 = ads2.readADC_SingleEnded(3);
  stopTime = micros();
  Serial.print("A0 to A11 input readings took "); Serial.print(stopTime - startTime); Serial.print(" uS");
  Serial.println();  

  // read 16 analog inputs
  startTime = micros();
  adc0 = ads0.readADC_SingleEnded(0);
  adc1 = ads0.readADC_SingleEnded(1);
  adc2 = ads0.readADC_SingleEnded(2);
  adc3 = ads0.readADC_SingleEnded(3);
  adc4 = ads1.readADC_SingleEnded(0);
  adc5 = ads1.readADC_SingleEnded(1);
  adc6 = ads1.readADC_SingleEnded(2);
  adc7 = ads1.readADC_SingleEnded(3);
  adc8 = ads2.readADC_SingleEnded(0);
  adc9 = ads2.readADC_SingleEnded(1);
  adc10 = ads2.readADC_SingleEnded(2);
  adc11 = ads2.readADC_SingleEnded(3);
  adc12 = ads3.readADC_SingleEnded(0);
  adc13 = ads3.readADC_SingleEnded(1);
  adc14 = ads3.readADC_SingleEnded(2);
  adc15 = ads3.readADC_SingleEnded(3);
  stopTime = micros();
  Serial.print("A0 to A15 input readings took "); Serial.print(stopTime - startTime); Serial.print(" uS");
  Serial.println();  Serial.println();

  delay(1500);
}
