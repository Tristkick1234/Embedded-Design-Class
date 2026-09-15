int FORWARD = 47;
int BACK = 48;
int R = 35;
int G = 36;
int B = 37;
int state = 0;

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GRN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(BUTTON47, INPUT_PULLDOWN);
  pinMode(BUTTON48, INPUT_PULLDOWN);
}

void loop() {
  if (digitalRead(FORWARD) == 1) {
    // check what state we're in
    if (state == 0) {
     //then go red 
    } else if (state == 1) {
      //Green
    } else if (state == 2) {
      //Blue
    } else if (state == 3) {
      //Yellow 
    } else {
      state = 0;
  }
    state++;
}
