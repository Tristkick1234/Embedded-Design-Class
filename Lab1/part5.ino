#define UP     11
#define DOWN   1
#define LEFT   7
#define RIGHT  3
#define CENTER 4

void setup() {
  // 115200 is much more stable for ESP32-C6 than 9600
  Serial.begin(115200); 
  
  pinMode(UP, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(CENTER, INPUT_PULLUP);
}

void loop() {
  String out = "";

  // 1. Check Center independently
  if (digitalRead(CENTER) == LOW) {
    out += "[Center]";
  }

  // 2. Check Vertical
  if (digitalRead(UP) == LOW) {
    out += "Top";
  } else if (digitalRead(DOWN) == LOW) {
    out += "Bottom";
  }

  // 3. Check Horizontal & bridge with a dash if a vertical direction was already added
  if (digitalRead(LEFT) == LOW) {
    if (out.length() > 0 && !out.endsWith(" ")) out += "-";
    out += "Left";
  } else if (digitalRead(RIGHT) == LOW) {
    if (out.length() > 0 && !out.endsWith(" ")) out += "-";
    out += "Right";
  }

  // 4. Print the combined result
  if (out != "") {
    Serial.println(out);
    delay(200); // Simple spam protection
  }
}
