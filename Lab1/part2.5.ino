void setup() {
  Serial.begin(115200);
  asm volatile (
    "lui  t0, 0x60090     \n"   // IO MUX base
    "lui  t1, 0x60091     \n"   // GPIO Matrix base

    // GPIO11 IO MUX: clear bits 7,8,9,12,13,14 (mask 0x7380)
    "lw   t2, 0x30(t0)    \n"
    "lui  t3, 0x7         \n"   // t3 = 0x7000
    "addi t3, t3, 0x380   \n"   // t3 = 0x7380
    "xori t3, t3, -1      \n"   // t3 = ~0x7380
    "and  t2, t2, t3      \n"
    "sw   t2, 0x30(t0)    \n"

    // GPIO11 IO MUX: set bits 7,9,12 (mask 0x1280)
    "lw   t2, 0x30(t0)    \n"
    "lui  t3, 0x1         \n"   // t3 = 0x1000
    "addi t3, t3, 0x280   \n"   // t3 = 0x1280
    "or   t2, t2, t3      \n"
    "sw   t2, 0x30(t0)    \n"

    // GPIO7 IO MUX: set bit 12 (MCU_SEL)
    "lw   t2, 0x20(t0)    \n"
    "lui  t3, 0x1         \n"   // t3 = 0x1000
    "or   t2, t2, t3      \n"
    "sw   t2, 0x20(t0)    \n"

    // GPIO7 output enable
    "addi t2, x0, 128     \n"
    "sw   t2, 0x24(t1)    \n"
    : : : "t0", "t1", "t2", "t3", "memory"
  );
}

void loop() {
  asm volatile (
    "lui  t1, 0x60091     \n"
    "lw   t2, 0x3C(t1)    \n"   // GPIO_IN_REG
    "addi t3, x0, 1       \n"
    "slli t3, t3, 11      \n"   // t3 = 2048 = 1<<11
    "and  t2, t2, t3      \n"
    "beq  t2, x0, 1f      \n"   // bit clear -> LED off

    "addi t4, x0, 128     \n"
    "sw   t4, 0x08(t1)    \n"   // OUT_W1TS: LED on
    "jal  x0, 2f          \n"

    "1:                   \n"
    "addi t4, x0, 128     \n"
    "sw   t4, 0x0C(t1)    \n"   // OUT_W1TC: LED off

    "2:                   \n"
    : : : "t1", "t2", "t3", "t4", "memory"
  );
}
