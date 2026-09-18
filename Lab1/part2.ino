volatile uint32_t* MY_GPIO_OUTPUT_EN  = (volatile uint32_t*) 0x60091024; 
volatile uint32_t* MY_GPIO_OUTPUT_SET = (volatile uint32_t*) 0x60091008; 
volatile uint32_t* MY_GPIO_OUTPUT_CLR = (volatile uint32_t*) 0x6009100C; 
volatile uint32_t* MY_GPIO_INPUT      = (volatile uint32_t*) 0x6009103C; 

volatile uint32_t* MY_IO_MUX_GPIO11   = (volatile uint32_t*) 0x60090030; // Button
volatile uint32_t* MY_IO_MUX_GPIO7    = (volatile uint32_t*) 0x60090020; // LED

void setup() {
  Serial.begin(9600);
  *MY_IO_MUX_GPIO11 &= ~(7 << 12);
  *MY_IO_MUX_GPIO11 &= ~(1 << 9);
  *MY_IO_MUX_GPIO11 &= ~(1 << 8);
  *MY_IO_MUX_GPIO11 &= ~(1 << 7);
  //clear

  *MY_IO_MUX_GPIO11 |=  ((1 << 12) | (1 << 9) | (1 << 7));
  *MY_IO_MUX_GPIO7 |= (1 << 12);
  
  //enable output in GPIO matrix
  *MY_GPIO_OUTPUT_EN = (1 << 7); 
}

void loop() {
  
  Serial.println(*MY_GPIO_INPUT & (1 << 11));
  //0 if low, 2048 if high;
  if ((*MY_GPIO_INPUT & (1 << 11)) == 0) {
    //If button is LOW, turn LED OFF 
    *MY_GPIO_OUTPUT_CLR = (1 << 7);

  } else {
    //If button is HIGH, turn LED ON 
    *MY_GPIO_OUTPUT_SET = (1 << 7);
    
  }
  delay(100);
}
