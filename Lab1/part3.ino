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
    delay(500);
    state++;
    if (state > 3) {
      state = 0;
    }
    Serial.println("forward");
    Serial.println(state);
    // check what state we're in
    if (state == 0) {
      //RED
     digitalWrite(R, HIGH);
     digitalWrite(G, LOW);
     digitalWrite(B, LOW);
    } 
    if (state == 1) {
      //GREEN
      digitalWrite(R, LOW);
      digitalWrite(G, HIGH);
      digitalWrite(B, LOW);
    } 
    if (state == 2) {
      //BLUE
      digitalWrite(R, LOW);
      digitalWrite(G, LOW);
      digitalWrite(B, HIGH);
    } 
    if (state == 3) {
      //YELLOW
      digitalWrite(R, HIGH);
      digitalWrite(G, HIGH);
      digitalWrite(B, LOW);
    } 
  }
  
  if (digitalRead(BACK) == 1) {
    delay(500);
    state--;
    if (state < 0) {
      state = 3;
    } 
    Serial.println("back");
    Serial.println(state);
    if (state == 3) {
      //YELLOW
      digitalWrite(R, HIGH);
      digitalWrite(G, HIGH);
      digitalWrite(B, LOW);
    } 
    if (state == 2) {
      //BLUE
      digitalWrite(R, LOW);
      digitalWrite(G, LOW);
      digitalWrite(B, HIGH);
    }
    if (state == 1) {
      //GREEN
      digitalWrite(R, LOW);
      digitalWrite(G, HIGH);
      digitalWrite(B, LOW);
    } 
    if (state == 0) {
      //RED
      digitalWrite(R, HIGH);
      digitalWrite(G, LOW);
      digitalWrite(B, LOW);
    }
  }
  // Serial.println("Check Serial Monitor");
}
