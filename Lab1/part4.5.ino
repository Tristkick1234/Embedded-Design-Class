volatile int state = -1;
volatile uint32_t lastForward = 0;
volatile uint32_t lastForwardCycles = 0;

// 50ms if CPU is actually 240 MHz
const uint32_t debounceCycles = 12000000;

void loop() {

  asm volatile(

    // --------------------------------
    // a2 = GPIO base
    // --------------------------------
    "movi a2, 0x60004000\n"

    // Read GPIO_IN1_REG
    "l32i a3, a2, 0x40\n"

    // Build GPIO47 mask: 1 << 15
    "movi a4, 1\n"
    "slli a4, a4, 15\n"

    // isolate GPIO47
    "and a3, a3, a4\n"

    // if current == 0, button isn't pressed
    "beqz a3, forward_not_pressed\n"


    // --------------------------------
    // Current = HIGH
    // Check previous state
    // --------------------------------

    "l32i a5, %0, 0\n"

    // if previous != 0, already held
    "bnez a5, forward_done\n"


    // --------------------------------
    // New rising edge
    // Check debounce time
    // --------------------------------

    "rsr.ccount a6\n"

    // previous accepted cycle count
    "l32i a7, %1, 0\n"

    // elapsed = now - previous
    "sub a8, a6, a7\n"

    // load debounce threshold
    "l32i a9, %2, 0\n"

    // if elapsed < threshold,
    // don't accept yet
    "bltu a8, a9, forward_done\n"


    // --------------------------------
    // Accept press
    // --------------------------------

    // save new cycle count
    "s32i a6, %1, 0\n"

    // load state
    "l32i a10, %3, 0\n"

    // state++
    "addi a10, a10, 1\n"

    // if state <= 3 we're done
    "movi a11, 3\n"
    "bge a11, a10, save_state\n"

    // otherwise wrap to 0
    "movi a10, 0\n"

    "save_state:\n"
    "s32i a10, %3, 0\n"

    "j forward_done\n"


    // --------------------------------
    // Current button is LOW
    // --------------------------------

    "forward_not_pressed:\n"
    "movi a5, 0\n"
    "s32i a5, %0, 0\n"
    "j finished\n"


    // --------------------------------
    // Save HIGH as previous
    // --------------------------------

    "forward_done:\n"
    "movi a5, 1\n"
    "s32i a5, %0, 0\n"

    "finished:\n"

    :
    : "r"(&lastForward),
      "r"(&lastForwardCycles),
      "r"(&debounceCycles),
      "r"(&state)
    : "a2", "a3", "a4", "a5",
      "a6", "a7", "a8", "a9",
      "a10", "a11", "memory"
  );
}
