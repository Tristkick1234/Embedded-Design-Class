// =======================================
// ESP32-S3 BASE ADDRESSES
// =======================================

uint32_t GPIO_BASE_ADDR   = 0x60004000;
uint32_t IO_MUX_BASE_ADDR = 0x60009000;


// =======================================
// IO MUX pointers
// =======================================

volatile uint32_t* MY_IO_MUX_GPIO35 =
    (volatile uint32_t*) 0x60009090;

volatile uint32_t* MY_IO_MUX_GPIO36 =
    (volatile uint32_t*) 0x60009094;

volatile uint32_t* MY_IO_MUX_GPIO37 =
    (volatile uint32_t*) 0x60009098;

volatile uint32_t* MY_IO_MUX_GPIO47 =
    (volatile uint32_t*) 0x600090C0;

volatile uint32_t* MY_IO_MUX_GPIO48 =
    (volatile uint32_t*) 0x600090C4;


// GPIO32-48 register masks
const uint32_t R_MASK       = (1u << (35 - 32)); // bit 3
const uint32_t G_MASK       = (1u << (36 - 32)); // bit 4
const uint32_t B_MASK       = (1u << (37 - 32)); // bit 5

const uint32_t FORWARD_MASK = (1u << (47 - 32)); // bit 15
const uint32_t BACK_MASK    = (1u << (48 - 32)); // bit 16


int state = -1;


// =======================================
// No-delay button handling
// =======================================

int lastForward = 0;
int lastBack = 0;

unsigned long lastForwardTime = 0;
unsigned long lastBackTime = 0;

const unsigned long debounceTime = 50;


// =======================================
// Configure IO MUX output using Xtensa
// =======================================

void configureOutput(volatile uint32_t* mux) {

  asm volatile(

    // Load current IO MUX value
    "l32i a3, %0, 0\n"

    // Build mask: 7 << 12
    "movi a4, 7\n"
    "slli a4, a4, 12\n"

    // invert mask
    "movi a5, -1\n"
    "xor a4, a4, a5\n"

    // clear MCU_SEL bits
    "and a3, a3, a4\n"

    // set MCU_SEL = 1
    "movi a4, 1\n"
    "slli a4, a4, 12\n"
    "or a3, a3, a4\n"

    // store back
    "s32i a3, %0, 0\n"

    :
    : "r"(mux)
    : "a3", "a4", "a5", "memory"
  );
}


// =======================================
// Configure INPUT_PULLDOWN using Xtensa
// =======================================

void configureInputPulldown(volatile uint32_t* mux) {

  asm volatile(

    // Read IO MUX register
    "l32i a3, %0, 0\n"


    // -----------------------
    // Clear MCU_SEL bits 14:12
    // -----------------------

    "movi a4, 7\n"
    "slli a4, a4, 12\n"
    "movi a5, -1\n"
    "xor a4, a4, a5\n"
    "and a3, a3, a4\n"


    // -----------------------
    // Clear bits 9,8,7
    // IE / WPU / WPD
    // -----------------------

    "movi a4, 7\n"
    "slli a4, a4, 7\n"
    "movi a5, -1\n"
    "xor a4, a4, a5\n"
    "and a3, a3, a4\n"


    // -----------------------
    // Set MCU_SEL bit 12
    // -----------------------

    "movi a4, 1\n"
    "slli a4, a4, 12\n"
    "or a3, a3, a4\n"


    // -----------------------
    // Set input enable bit 9
    // -----------------------

    "movi a4, 1\n"
    "slli a4, a4, 9\n"
    "or a3, a3, a4\n"


    // -----------------------
    // Set pull-down bit 7
    // -----------------------

    "movi a4, 1\n"
    "slli a4, a4, 7\n"
    "or a3, a3, a4\n"


    // Store configured register
    "s32i a3, %0, 0\n"

    :
    : "r"(mux)
    : "a3", "a4", "a5", "memory"
  );
}


// =======================================
// Set RGB output using Xtensa
// =======================================

void setColor(uint32_t clearMask, uint32_t setMask) {

  asm volatile(

    // GPIO_OUT1_W1TC_REG = base + 0x18
    "s32i %0, %2, 0x18\n"

    // GPIO_OUT1_W1TS_REG = base + 0x14
    "s32i %1, %2, 0x14\n"

    :
    : "r"(clearMask),
      "r"(setMask),
      "r"(GPIO_BASE_ADDR)
    : "memory"
  );
}


void setup() {

  Serial.begin(9600);


  // =======================================
  // Configure RGB outputs
  // =======================================

  configureOutput(MY_IO_MUX_GPIO35);
  configureOutput(MY_IO_MUX_GPIO36);
  configureOutput(MY_IO_MUX_GPIO37);


  // =======================================
  // Configure buttons as INPUT_PULLDOWN
  // =======================================

  configureInputPulldown(MY_IO_MUX_GPIO47);
  configureInputPulldown(MY_IO_MUX_GPIO48);


  // =======================================
  // Enable GPIO35,36,37 as outputs
  // GPIO_ENABLE1_W1TS_REG = +0x30
  // =======================================

  asm volatile(

    // bits 3,4,5 = GPIO35,36,37
    "movi a3, 0x38\n"

    // enable outputs
    "s32i a3, %0, 0x30\n"

    // start RGB LED off
    // GPIO_OUT1_W1TC_REG = +0x18
    "s32i a3, %0, 0x18\n"

    :
    : "r"(GPIO_BASE_ADDR)
    : "a3", "memory"
  );
}


void loop() {

  uint32_t inputRegister;


  // =======================================
  // Read GPIO_IN1_REG using Xtensa
  // =======================================

  asm volatile(

    // GPIO_IN1_REG = base + 0x40
    "l32i %0, %1, 0x40\n"

    : "=r"(inputRegister)
    : "r"(GPIO_BASE_ADDR)
    : "memory"
  );


  int forward = ((inputRegister & FORWARD_MASK) != 0);
  int back    = ((inputRegister & BACK_MASK) != 0);


  unsigned long now = millis();


  // =======================================
  // FORWARD button
  // Detect LOW -> HIGH
  // =======================================

  if (forward == 1 &&
      lastForward == 0 &&
      (now - lastForwardTime >= debounceTime)) {

    state++;

    if (state > 3) {
      state = 0;
    }

    lastForwardTime = now;

    Serial.println("forward");
    Serial.println(state);
  }


  // =======================================
  // BACK button
  // Detect LOW -> HIGH
  // =======================================

  if (back == 1 &&
      lastBack == 0 &&
      (now - lastBackTime >= debounceTime)) {

    state--;

    if (state < 0) {
      state = 3;
    }

    lastBackTime = now;

    Serial.println("back");
    Serial.println(state);
  }


  lastForward = forward;
  lastBack = back;


  // =======================================
  // RGB STATES
  // =======================================

  if (state == 0) {

    // RED
    setColor(G_MASK | B_MASK,
             R_MASK);
  }

  else if (state == 1) {

    // GREEN
    setColor(R_MASK | B_MASK,
             G_MASK);
  }

  else if (state == 2) {

    // BLUE
    setColor(R_MASK | G_MASK,
             B_MASK);
  }

  else if (state == 3) {

    // YELLOW
    setColor(B_MASK,
             R_MASK | G_MASK);
  }

  else {

    // initial state = OFF
    setColor(R_MASK | G_MASK | B_MASK,
             0);
  }
}
