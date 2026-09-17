volatile uint32_t* MUX_GPIO11 = (volatile uint32_t*)(0x60090000 + 0x0030);
volatile uint32_t* IN_REG = (volatile uint32_t*)(0x60091000 + 0x003C);

//add offset to base addr
void setup(){
  Serial.begin(9600);
  
  //get gpio11[value]
 
  *MUX_GPIO11 &= ~((7 << 12) | (1 << 9) | (1 << 8) | (1 << 7));
  //set  mcusel, input enable, pulldown
  *MUX_GPIO11 |= ((1 << 12) | (1 << 9) | (1 << 7));
}

void loop() {
  int button = (*IN_REG >> 11) & 1;

  Serial.println(button);
  delay(100);
}


