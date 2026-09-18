const uint8_t ledChannel = 0;     
const uint8_t freq = 4000;        //Frequency in Hz (5 kHz)
const uint8_t resolution = 8;     //8-bit resolution (0-255)
const uint8_t pin = 18;           //Output GPIO pin

void setup() {
  ledcAttach(pin, freq, resolution);
  
}

void loop() {
  ledcWrite(pin, 128); 
}
