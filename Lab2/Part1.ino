// ECE 4180 Lab 2 Part 1: ESP32-S3 -> CH9328 (Mode 3, raw 8-byte HID, 9600 baud)
// CH9328 DIP: SW3 ON, SW4 ON, SW2 OFF (replug after changing)
//
// Wiring (ESP32-S3 DevKitC):
//   GPIO17 (U1_TXD) -> CH9328 RX
//   GND             -> CH9328 GND
//   CH9328 VCC and RST: NOT connected (USB powers the CH9328)
//   Nav switch (COM -> GND, INPUT_PULLUP):
//     up=GPIO4, down=GPIO5, left=GPIO6, right=GPIO7, center=GPIO15

const int TX_PIN = 17;  // UART1 default TX; avoids GPIO35-37 (PSRAM on some S3 modules)

const int pins[5] = {4, 5, 6, 7, 15};
const uint8_t arrows[4] = {0x52, 0x51, 0x50, 0x4F};  // Up, Down, Left, Right
const uint8_t konami[10] = {0x52, 0x52, 0x51, 0x51, 0x50, 0x4F, 0x50, 0x4F, 0x05, 0x04};
const uint8_t RESET_KEY = 0x1D;  // 'Z': harmless key that resets the game's Konami detector

int count = 0;

void tap(uint8_t key) {
  uint8_t press[8] = {0, 0, key, 0, 0, 0, 0, 0};
  uint8_t release[8] = {0};

  Serial1.write(press, 8);
  delay(50);
  Serial1.write(release, 8);
  delay(100);
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, -1, TX_PIN);  // RX unused (CH9328 is one-way)

  // The CH9328 stays powered by USB when the ESP32 resets, so make sure no key is left held down
  uint8_t release[8] = {0};
  Serial1.write(release, 8);

  for (int i = 0; i < 5; i++) {
    pinMode(pins[i], INPUT_PULLUP);
  }
}

void loop() {
  for (int i = 0; i < 5; i++) {
    if (digitalRead(pins[i]) == LOW) {
      delay(20);  // debounce

      if (digitalRead(pins[i]) == LOW) {
        Serial.printf("press %d on pin %d\n", ++count, pins[i]);

        if (i < 4) {
          tap(arrows[i]);
        } else {
          tap(RESET_KEY);  // clear any partial Konami progress in the game
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
