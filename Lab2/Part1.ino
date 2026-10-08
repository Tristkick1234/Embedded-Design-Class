// nav switch up = 4
// down = 5  left = 6  right = 7  center = 15

const int pins[5] = {4,5,6,7,15};
const uint8_t arrows[4] = {0x52,0x51,0x50,0x4F};
const uint8_t konami[10] = {0x52,0x52,0x51,0x51,0x50,0x4F,0x50,0x4F,0x05,0x04};

void tap(uint8_t key) {
  uint8_t press[8] = {0,0,key,0,0,0,0,0};
  uint8_t release[8] = {0};

  Serial1.write(press, 8);
  delay(50);
  Serial1.write(release, 8);
  delay(100);
}

void setup() {
  Serial1.begin(9600, SERIAL_8N1, 36, 35);

  for (int i = 0; i < 5; i++) {
    pinMode(pins[i], INPUT_PULLUP);
  }
}

void loop() {
  for (int i = 0; i < 5; i++) {
    if (digitalRead(pins[i]) == LOW) {
      delay(20);

      if (digitalRead(pins[i]) == LOW) {
        if (i < 4) {
          tap(arrows[i]);
        } else {
          for (int k = 0; k < 10; k++) {
            tap(konami[k]);
          }
        }

        while (digitalRead(pins[i]) == LOW) {
          delay(10);
        }
      }
    }
  }
}
