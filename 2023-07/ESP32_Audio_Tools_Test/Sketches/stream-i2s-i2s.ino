/**
 * ESP32 using library https://github.com/pschatzmann/arduino-audio-tools
 * Stream audio from two I2S microphone inputs (L+R on one channel) to I2S external DAC output on second channel
 * Original author Phil Schatzmann
 * @copyright GPLv3
 */

#include "AudioTools.h"
 
AudioInfo info(44100, 2, 16);   // set up audio paths
I2SStream in;
I2SStream out; 

StreamCopy copier( out, in); // copies sound from I2S input to I2S output

void setup(void) {  
  Serial.begin(115200);
  // change to Warning to improve the quality
  AudioLogger::instance().begin(Serial, AudioLogger::Warning); 

  // start I2S input
  Serial.println("starting I2S...");
  auto config_in = in.defaultConfig(RX_MODE);
  config_in.copyFrom(info); 
  config_in.i2s_format = I2S_STD_FORMAT;
  config_in.is_master = true;
  config_in.port_no = 0;   // i2s port number
  config_in.pin_bck = 4;   // i2s bit clock pin
  config_in.pin_ws = 16;   // i2s word select pin
  config_in.pin_data = 17; // i2s data pin
  in.begin(config_in);

  // start I2S out
  auto config_out = out.defaultConfig(TX_MODE);
  config_out.copyFrom(info); 
  config_out.i2s_format = I2S_STD_FORMAT;
  config_out.is_master = true;
  config_out.port_no = 1;   // i2s port number
  config_out.pin_bck = 14;  // i2s bit clock pin
  config_out.pin_ws = 15;   // i2s word select pin
  config_out.pin_data = 22; // i2s data pin
  out.begin(config_out);

  Serial.println("I2S started...");
}

void loop() {
  copier.copy();  // stream audio from input channel to output channel
}
