int BUTTON = 11;  // Data pin connected to onboard RGB LED for ESP32-c6
int LED = 7;

void setup() {
  //init GPIO pin and serial monitor
  Serial.begin(9600);
  pinMode(BUTTON, INPUT_PULLDOWN);
  pinMode(LED, OUTPUT);
}

void loop() {
  //Read values from pin 11. drive one side of button high.
  int button = digitalRead(BUTTON);
  digitalWrite(LED, button);
  Serial.println(button);
}
