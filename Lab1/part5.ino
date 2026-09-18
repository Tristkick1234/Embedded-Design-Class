
#define UP     11
#define DOWN   1
#define LEFT   7
#define RIGHT  3
#define CENTER 4

void setup() {
  Serial.begin(9600);
  
  pinMode(UP, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(CENTER, INPUT_PULLUP);
}

void loop() {
  String out = "";

  if (digitalRead(CENTER) == LOW) out += "Center ";
  if (digitalRead(UP) == LOW)     out += "Top-";
  if (digitalRead(DOWN) == LOW)   out += "Bottom-";
  if (digitalRead(LEFT) == LOW)   out += "Left";
  if (digitalRead(RIGHT) == LOW)  out += "Right";

  if (out.endsWith("-")) out.remove(out.length() - 1);

  if (out != "") {
    Serial.println(out);
    delay(200);
  }
}
