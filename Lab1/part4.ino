// =======================================
// ESP32-S3 GPIO MMIO registers
// GPIO32 - GPIO48 use the "1" registers
// =======================================

volatile uint32_t* MY_GPIO_OUTPUT_EN =
    (volatile uint32_t*) 0x60004030; // GPIO_ENABLE1_W1TS_REG

volatile uint32_t* MY_GPIO_OUTPUT_SET =
    (volatile uint32_t*) 0x60004014; // GPIO_OUT1_W1TS_REG

volatile uint32_t* MY_GPIO_OUTPUT_CLR =
    (volatile uint32_t*) 0x60004018; // GPIO_OUT1_W1TC_REG

volatile uint32_t* MY_GPIO_INPUT =
    (volatile uint32_t*) 0x60004040; // GPIO_IN1_REG


// =======================================
// IO MUX registers
// =======================================

// RGB LED outputs
volatile uint32_t* MY_IO_MUX_GPIO35 =
    (volatile uint32_t*) 0x60009090;

volatile uint32_t* MY_IO_MUX_GPIO36 =
    (volatile uint32_t*) 0x60009094;

volatile uint32_t* MY_IO_MUX_GPIO37 =
    (volatile uint32_t*) 0x60009098;

// Pushbutton inputs
volatile uint32_t* MY_IO_MUX_GPIO47 =
    (volatile uint32_t*) 0x600090C0;

volatile uint32_t* MY_IO_MUX_GPIO48 =
    (volatile uint32_t*) 0x600090C4;


// =======================================
// Bit positions inside GPIO32-48 registers
// =======================================

const uint32_t R_MASK = (1 << (35 - 32)); // bit 3
const uint32_t G_MASK = (1 << (36 - 32)); // bit 4
const uint32_t B_MASK = (1 << (37 - 32)); // bit 5

const uint32_t FORWARD_MASK = (1 << (47 - 32)); // bit 15
const uint32_t BACK_MASK    = (1 << (48 - 32)); // bit 16


int state = -1;


void setup() {

  Serial.begin(9600);


  // =======================================
  // GPIO35 = GPIO output
  // =======================================

  *MY_IO_MUX_GPIO35 &= ~(7 << 12);
  *MY_IO_MUX_GPIO35 |=  (1 << 12);


  // =======================================
  // GPIO36 = GPIO output
  // =======================================

  *MY_IO_MUX_GPIO36 &= ~(7 << 12);
  *MY_IO_MUX_GPIO36 |=  (1 << 12);


  // =======================================
  // GPIO37 = GPIO output
  // =======================================

  *MY_IO_MUX_GPIO37 &= ~(7 << 12);
  *MY_IO_MUX_GPIO37 |=  (1 << 12);


  // =======================================
  // GPIO47 = input with pull-down
  // =======================================

  *MY_IO_MUX_GPIO47 &= ~(7 << 12);
  *MY_IO_MUX_GPIO47 &= ~(1 << 9);
  *MY_IO_MUX_GPIO47 &= ~(1 << 8);
  *MY_IO_MUX_GPIO47 &= ~(1 << 7);

  *MY_IO_MUX_GPIO47 |=
      (1 << 12) |
      (1 << 9)  |
      (1 << 7);


  // =======================================
  // GPIO48 = input with pull-down
  // =======================================

  *MY_IO_MUX_GPIO48 &= ~(7 << 12);
  *MY_IO_MUX_GPIO48 &= ~(1 << 9);
  *MY_IO_MUX_GPIO48 &= ~(1 << 8);
  *MY_IO_MUX_GPIO48 &= ~(1 << 7);

  *MY_IO_MUX_GPIO48 |=
      (1 << 12) |
      (1 << 9)  |
      (1 << 7);


  // =======================================
  // Enable GPIO35, GPIO36, GPIO37 outputs
  // =======================================

  *MY_GPIO_OUTPUT_EN =
      R_MASK |
      G_MASK |
      B_MASK;


  // Start with LED off
  *MY_GPIO_OUTPUT_CLR =
      R_MASK |
      G_MASK |
      B_MASK;
}


void loop() {

  // =======================================
  // Read GPIO47 directly
  // =======================================

  if ((*MY_GPIO_INPUT & FORWARD_MASK) != 0) {

    delay(500);

    state++;

    if (state > 3) {
      state = 0;
    }

    Serial.println("forward");
    Serial.println(state);


    if (state == 0) {
      // RED
      *MY_GPIO_OUTPUT_CLR = G_MASK | B_MASK;
      *MY_GPIO_OUTPUT_SET = R_MASK;
    }

    if (state == 1) {
      // GREEN
      *MY_GPIO_OUTPUT_CLR = R_MASK | B_MASK;
      *MY_GPIO_OUTPUT_SET = G_MASK;
    }

    if (state == 2) {
      // BLUE
      *MY_GPIO_OUTPUT_CLR = R_MASK | G_MASK;
      *MY_GPIO_OUTPUT_SET = B_MASK;
    }

    if (state == 3) {
      // YELLOW
      *MY_GPIO_OUTPUT_CLR = B_MASK;
      *MY_GPIO_OUTPUT_SET = R_MASK | G_MASK;
    }
  }


  // =======================================
  // Read GPIO48 directly
  // =======================================

  if ((*MY_GPIO_INPUT & BACK_MASK) != 0) {

    delay(500);

    state--;

    if (state < 0) {
      state = 3;
    }

    Serial.println("back");
    Serial.println(state);


    if (state == 3) {
      // YELLOW
      *MY_GPIO_OUTPUT_CLR = B_MASK;
      *MY_GPIO_OUTPUT_SET = R_MASK | G_MASK;
    }

    if (state == 2) {
      // BLUE
      *MY_GPIO_OUTPUT_CLR = R_MASK | G_MASK;
      *MY_GPIO_OUTPUT_SET = B_MASK;
    }

    if (state == 1) {
      // GREEN
      *MY_GPIO_OUTPUT_CLR = R_MASK | B_MASK;
      *MY_GPIO_OUTPUT_SET = G_MASK;
    }

    if (state == 0) {
      // RED
      *MY_GPIO_OUTPUT_CLR = G_MASK | B_MASK;
      *MY_GPIO_OUTPUT_SET = R_MASK;
    }
  }
}
