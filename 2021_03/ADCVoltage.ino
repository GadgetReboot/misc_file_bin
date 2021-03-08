

#define VREF 5.136  // what reference voltage is being used for the ADC?
//#define VREF 2.493

void setup() {
  Serial.begin(9600);
  // (Don't connect a voltage to AREF pin if not using EXTERNAL reference in sketch)
  analogReference(DEFAULT);   // DEFAULT=VCC 5v    EXTERNAL=AREF pin input voltage
  //analogReference(EXTERNAL);
}

void loop() {
  // input voltage = number of steps * voltage per step
  // eg. VREF=5V, ADC=10 bit=1024 steps,  5v/1024=4.88mV/step
  //   if there is 2.5v at A0 input, analog reading = 512
  //   so measured voltage = 512 steps * 4.88mv/step = 2.5V
  int reading = analogRead(A0);                // read A0 voltage
  float vMeasured = reading * VREF / 1024.0;   // calculate voltage
  Serial.print("Analog reading and Voltage "); 
  Serial.print(reading); Serial.print(", ");    
  Serial.print(vMeasured, 5);                  // print 5 digits after decimal
  Serial.println("V");
}
