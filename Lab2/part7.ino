#include <Wire.h>
#include <Adafruit_MPR121.h>

#define SDA_PIN 6
#define SCL_PIN 7

Adafruit_MPR121 cap = Adafruit_MPR121();

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); } 
  
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  
  Serial.println("Starting raw data stream for all pads...");
  delay(1000);
}

void loop() {
  // Loop through all 12 capacitive sensing inputs
  for (uint8_t i = 0; i < 12; i++) {
    if (cap.filteredData(i) < 10) {
      Serial.print(i);
      Serial.println(" was pressed.");
    }
  }
  delay(100); 
}
