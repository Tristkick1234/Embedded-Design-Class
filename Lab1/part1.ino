int LED_PIN = 11;  // Data pin connected to onboard RGB LED for ESP32-c6
void setup() {
  //init GPIO pin and serial monitor
  Serial.begin(9600);
  pinMode(LED_PIN, INPUT_PULLDOWN);
}

void loop() {
  //Read values from pin 11. drive one side of button high.
  int button = digitalRead(LED_PIN);
  Serial.println(button);
}
