uint32_t* MY_GPIO_OUTPUT_EN = (uint32_t*) 0x60004030; // GPIO_ENABLE1_W1TS_REG
uint32_t* MY_GPIO_OUTPUT_SET = (uint32_t*) 0x60004014; // GPIO_OUT1_W1TS_REG
uint32_t* MY_GPIO_OUTPUT_CLR = (uint32_t*) 0x60004018; // GPIO_OUT1_W1TC_REG
uint32_t* MY_GPIO_INPUT = (uint32_t*) 0x60004040; // GPIO_IN1_REG

uint32_t* MY_IO_MUX_GPIO35 = (uint32_t*) 0x60009090;
uint32_t* MY_IO_MUX_GPIO36 = (uint32_t*) 0x60009094;
uint32_t* MY_IO_MUX_GPIO37 = (uint32_t*) 0x60009098;
uint32_t* MY_IO_MUX_GPIO47 = (uint32_t*) 0x600090C0;
uint32_t* MY_IO_MUX_GPIO48 = (uint32_t*) 0x600090C4;

// positions inside GPIO32-48 registers
// R =gpio 35
// g = gpio 36
//b = gpio 37
// forward =47
//back = 48
const uint32_t R = (1 << (35 - 32)); // bit 3
const uint32_t G = (1 << (36 - 32)); // bit 4
const uint32_t B = (1 << (37 - 32)); // bit 5

const uint32_t FORWARD = (1 << (47 - 32)); // bit 15
const uint32_t BACK = (1 << (48 - 32)); // bit 16

int state = -1;

void setup() {
  Serial.begin(9600);
 // set red to function 1
  *MY_IO_MUX_GPIO35 &= ~(7 << 12);
  *MY_IO_MUX_GPIO35 |= (1 << 12);
 //set green to function 1 gpio
  *MY_IO_MUX_GPIO36 &= ~(7 << 12);
  *MY_IO_MUX_GPIO36 |= (1 << 12);
  //set blue to function 1
  *MY_IO_MUX_GPIO37 &= ~(7 << 12);
  *MY_IO_MUX_GPIO37 |= (1 << 12);

// input pulldown on pin 47
  *MY_IO_MUX_GPIO47 &= ~(7 << 12);
  *MY_IO_MUX_GPIO47 &= ~(1 << 9);
  *MY_IO_MUX_GPIO47 &= ~(1 << 8);
  *MY_IO_MUX_GPIO47 &= ~(1 << 7);

  *MY_IO_MUX_GPIO47 |= (1 << 12) | (1 << 9) | (1 << 7);

// pulldown on pin 48
  *MY_IO_MUX_GPIO48 &= ~(7 << 12);
  *MY_IO_MUX_GPIO48 &= ~(1 << 9);
  *MY_IO_MUX_GPIO48 &= ~(1 << 8);
  *MY_IO_MUX_GPIO48 &= ~(1 << 7);

  *MY_IO_MUX_GPIO48 |=(1 << 12) | (1 << 9) | (1 << 7);

  *MY_GPIO_OUTPUT_EN = R | G | B;
  // Start with LED off
  *MY_GPIO_OUTPUT_CLR = R | G | B;
}

void loop() {
  if ((*MY_GPIO_INPUT & FORWARD) != 0) {
    delay(500);
    state++;

    if (state > 3) {
      state = 0;
    }
    Serial.println("forward");
    Serial.println(state);

    if (state == 0) {
      // RED
      *MY_GPIO_OUTPUT_CLR = G | B;
      *MY_GPIO_OUTPUT_SET = R;
    }
    if (state == 1) {
      // GREEN
      *MY_GPIO_OUTPUT_CLR = R | B;
      *MY_GPIO_OUTPUT_SET = G;
    }
    if (state == 2) {
      // BLUE
      *MY_GPIO_OUTPUT_CLR = R | G;
      *MY_GPIO_OUTPUT_SET = B;
    }
    if (state == 3) {
      // YELLOW
      *MY_GPIO_OUTPUT_CLR = B;
      *MY_GPIO_OUTPUT_SET = R | G;
    }
  }

  if ((*MY_GPIO_INPUT & BACK) != 0) {
    delay(500);
    state--;
    if (state < 0) {
      state = 3;
    }

    Serial.println("back");
    Serial.println(state);
      
    if (state == 3) {
      // YELLOW
      *MY_GPIO_OUTPUT_CLR = B;
      *MY_GPIO_OUTPUT_SET = R | G;
    }
    if (state == 2) {
      // BLUE
      *MY_GPIO_OUTPUT_CLR = R | G;
      *MY_GPIO_OUTPUT_SET = B;
    }
    if (state == 1) {
      // GREEN
      *MY_GPIO_OUTPUT_CLR = R | B;
      *MY_GPIO_OUTPUT_SET = G;
    }
    if (state == 0) {
      // RED
      *MY_GPIO_OUTPUT_CLR = G | B;
      *MY_GPIO_OUTPUT_SET = R;
    }
  }
}
