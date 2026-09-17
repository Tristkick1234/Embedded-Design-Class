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
  // 2. Fixed: Added (1 << 12) to actually route the pin to the GPIO matrix
  *MY_IO_MUX_GPIO11 |=  ((1 << 12) | (1 << 9) | (1 << 7));


  // --- CONFIGURE GPIO 7 (OUTPUT LED) ---
  // 3. Fixed: Added IO MUX configuration for the output pin
  // Clear function bits (12-14)
  *MY_IO_MUX_GPIO7 &= ~(7 << 12);
  // Route pin to GPIO matrix (MCU_SEL = 1)
  *MY_IO_MUX_GPIO7 |= (1 << 12);
  
  // Now logically enable the output in the GPIO Matrix
  *MY_GPIO_OUTPUT_EN = (1 << 7); 
}

void loop() {
  // Your friend's read logic (*MY_GPIO_INPUT & (1 << 11)) works perfectly.
  // It returns 0 if unpressed, and 2048 if pressed.
  
  if ((*MY_GPIO_INPUT & (1 << 11)) == 0) {
    // If button is LOW, turn LED OFF (Write 1 to clear register)
    *MY_GPIO_OUTPUT_CLR = (1 << 7);
    Serial.println(*MY_GPIO_INPUT);
    delay(100);
  } else {
    // If button is HIGH, turn LED ON (Write 1 to set register)
    *MY_GPIO_OUTPUT_SET = (1 << 7);
    Serial.println(*MY_GPIO_INPUT);
    delay(100);
  }
}
