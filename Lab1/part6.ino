#define POT 3
#define MOTOR 11

void setup() {
  //set pin mode
  Serial.begin(9600);
  pinMode(POT, INPUT);
  pinMode(MOTOR, OUTPUT);
}

void loop() {
  int voltage = analogRead(POT);
  int motorSpeed = map(voltage, 0, 1023, 0, 255);
  Serial.println(motorSpeed);
  analogWrite(MOTOR, voltage);
  delay(100);
}
