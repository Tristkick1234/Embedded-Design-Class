// // initializing this registers
// int* GPIO_OUTPUT_EN = (int*) 0x60091024; // GPIO_ENABLE_W1TS_REG
// int* GPIO_OUTPUT_SET = (int*) 0x60091008; //GPIO_OUT_W1TS_REG
// int* GPIO_OUTPUT_CLR = (int*) 0x6009100C; // clear register
// int* GPIO_INPUT = (int*) 0x6009103C; //input
// int* IO_MUX_GPIO11 = (int*) 0x60090030;

// void setup() {
//   *GPIO_OUTPUT_EN = (1 << 7); //setting the output to be gpio7
//   *IO_MUX_GPIO11 |=  (1 << 9) | (1 << 7);
//   *IO_MUX_GPIO11 &= ~(1 << 8);
// }

// void loop() {
//   if ((*GPIO_INPUT & (1 << 11)) == 0) {
//     *GPIO_OUTPUT_CLR = (1 << 7);
//   } else {
//     *GPIO_OUTPUT_SET = (1 << 7);
//   }
// }


volatile int* GPIO_OUTPUT_EN  = (volatile int*) 0x60091024;
volatile int* GPIO_OUTPUT_SET = (volatile int*) 0x60091008;
volatile int* GPIO_OUTPUT_CLR = (volatile int*) 0x6009100C;
volatile int* GPIO_INPUT      = (volatile int*) 0x6009103C;
volatile int* IO_MUX_GPIO11   = (volatile int*) 0x60090030;

void setup() {
  *GPIO_OUTPUT_EN = (1 << 7);
  *IO_MUX_GPIO11 |=  (1 << 9) | (1 << 7);
  *IO_MUX_GPIO11 &= ~(1 << 8);
}

void loop() {
  if ((*GPIO_INPUT & (1 << 11)) == 0) {
    *GPIO_OUTPUT_CLR = (1 << 7);
  } else {
    *GPIO_OUTPUT_SET = (1 << 7);
  }
}
