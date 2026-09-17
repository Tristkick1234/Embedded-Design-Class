int FORWARD = 47;
int BACK = 48;
int R = 35;
int G = 36;
int B = 37;
int state = -1;

void setup() {
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(FORWARD, INPUT_PULLDOWN);
  pinMode(BACK, INPUT_PULLDOWN);
  Serial.begin(9600);
}

void loop() {
  if (digitalRead(FORWARD) == 1) {
    delay(1000);
    Serial.println("forward");
    Serial.println(state);
    state++;
    if (state > 3) {
      state = 0;
    }
    // check what state we're in
    if (state == 0) {
     digitalWrite(R, HIGH);
    } 
    if (state == 1) {
      digitalWrite(G, HIGH);
    } 
    if (state == 2) {
      digitalWrite(B, HIGH);
    } 
    if (state == 3) {
      digitalWrite(R, HIGH);
      digitalWrite(G, HIGH);
    } 
  }
  
  if (digitalRead(BACK) == 1) {
    delay(1000);
    Serial.println("back");
    Serial.println(state);
    state--;
    if (state < 0) {
      state = 3;
    } 
    if (state == 3) {
      digitalWrite(R, HIGH);
      digitalWrite(G, HIGH);
    } 
    if (state == 2) {
      digitalWrite(B, HIGH);
    }
    if (state == 1) {
      digitalWrite(G, HIGH);
    } 
    if (state == 0) {
      digitalWrite(R, HIGH);
    }
  }
  // Serial.println("Check Serial Monitor");
}
