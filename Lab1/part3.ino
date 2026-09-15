int FORWARD = 47;
int BACK = 48;
int R = 35;
int G = 36;
int B = 37;
int state = 0;

void setup() {
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(FORWARD, INPUT_PULLDOWN);
  pinMode(BACK, INPUT_PULLDOWN);
}

void loop() {
  if (digitalRead(FORWARD) == 1) {
    state++;
    if (state > 3) {
      state = 0;
    }
    // check what state we're in
    if (state == 0) {
     //then go red 
    } else if (state == 1) {
      //Green
    } else if (state == 2) {
      //Blue
    } else if (state == 3) {
      //Yellow 
    } 
  } else if (digitalRead(BACK) == 1) {
    state--;
    if (state < 0) {
      state = 3;
    } 
    if (state == 3) {
      // Yellow
    } else if (state == 2) {
      //Blue
    } else if (state == 1) {
      //Green
    } else if (state == 0) {
      //red
    }
  }
}
