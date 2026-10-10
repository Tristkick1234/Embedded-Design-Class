#include <Wire.h>

#define SDA_PIN 6
#define SCL_PIN 7
#define DAC_ADDR 0x60            // 7-bit address: 1100 000 (A0 = GND)

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);         // 100 kHz -> 10 µs per bit, easy to read
}

void loop() {
  Wire.beginTransmission(DAC_ADDR);   // START, then 0xC0 (addr<<1 | W=0)
  Wire.endTransmission();             // DAC ACKs, then STOP
  delay(5);                           // gap so the scope re-triggers cleanly
}
