# ECE 4180 Lab 1 Part 4.5 - Part 4 in Xtensa assembly (ESP32-S3, LX7)
#
# Outputs: GPIO35 = R, GPIO36 = G, GPIO37 = B
# Inputs:  GPIO47 = forward, GPIO48 = backward, internal pull-ups, buttons to GND
#
# No delay() anywhere. Debounce is done by timestamping transitions with the
# CCOUNT special register (the CPU cycle counter) and comparing elapsed cycles,
# which is the non-blocking sampling approach from the Digital I/O lecture, just
# with a hardware counter instead of millis().
#
# Register map (S3 TRM Table 4.3-3, section 6.14):
#   IO MUX base      0x6000_9000,  IO_MUX_GPIOn_REG = base + 0x04 + 4*n
#     GPIO35 0x60009090   GPIO36 0x60009094   GPIO37 0x60009098
#     GPIO47 0x600090C0   GPIO48 0x600090C4
#   GPIO matrix base 0x6000_4000
#     OUT1_W1TS    +0x0014     OUT1_W1TC   +0x0018
#     ENABLE1_W1TS +0x0030     IN1         +0x0040
#     FUNC35_OUT_SEL_CFG 0x600045E0 (+4 per pin)
#
# Bits in the high bank are GPIO number minus 32:
#   R = bit3 (0x08)   G = bit4 (0x10)   B = bit5 (0x20)
#   FWD = bit15       BACK = bit16

    .equ CFG_OUT,   0x1800      # MCU_SEL=1 (GPIO), FUN_DRV=2
    .equ CFG_IN,    0x1B00      # MCU_SEL=1, FUN_DRV=2, FUN_IE=1, FUN_WPU=1
    .equ OUT_SEL,   256         # simple GPIO output index on the S3
    .equ RGB_MASK,  0x38        # bits 3,4,5

    .equ OFF_OUT1_W1TS,    0x14
    .equ OFF_OUT1_W1TC,    0x18
    .equ OFF_ENABLE1_W1TS, 0x30
    .equ OFF_IN1,          0x40

    .equ OFF_IOMUX_G36,  0x04
    .equ OFF_IOMUX_G37,  0x08
    .equ OFF_IOMUX_G47,  0x30
    .equ OFF_IOMUX_G48,  0x34

# 25 ms at 240 MHz. Change if you change the CPU clock in Tools.
    .equ DEBOUNCE_CYCLES, 6000000

# ---------------------------------------------------------------------------
    .data
    .align 4
g_vars:
    .word 0             # +0  state (0..3)
    .word 0             # +4  fwd.stable
    .word 0             # +8  fwd.lastRead
    .word 0             # +12 fwd.lastChange
    .word 0             # +16 back.stable
    .word 0             # +20 back.lastRead
    .word 0             # +24 back.lastChange

    .align 4
color_tab:
    .word 0x08          # 0 Red     R
    .word 0x10          # 1 Green   G
    .word 0x20          # 2 Blue    B
    .word 0x18          # 3 Yellow  R+G

# ---------------------------------------------------------------------------
    .section .text

    .literal .L_iomux35,   0x60009090
    .literal .L_gpiobase,  0x60004000
    .literal .L_funcsel,   0x600045E0
    .literal .L_cfg_out,   CFG_OUT
    .literal .L_cfg_in,    CFG_IN
    .literal .L_debounce,  DEBOUNCE_CYCLES
    .literal .L_vars,      g_vars
    .literal .L_colortab,  color_tab

# ---------------------------------------------------------------------------
# void xtensa_gpio_init(void)
# ---------------------------------------------------------------------------
    .align 4
    .global xtensa_gpio_init
    .type   xtensa_gpio_init, @function
xtensa_gpio_init:
    entry a1, 32

    # IO MUX. GPIO47/48 reset to SPICLK_P_DIFF / SPICLK_N_DIFF on Function 0,
    # so MCU_SEL must be moved to 1 or GPIO_IN1 never sees the buttons.
    l32r a2, .L_iomux35
    l32r a3, .L_cfg_out
    s32i a3, a2, 0                      # GPIO35
    s32i a3, a2, OFF_IOMUX_G36          # GPIO36
    s32i a3, a2, OFF_IOMUX_G37          # GPIO37
    l32r a3, .L_cfg_in
    s32i a3, a2, OFF_IOMUX_G47          # GPIO47
    s32i a3, a2, OFF_IOMUX_G48          # GPIO48

    # Route GPIO_OUT1_REG bits to the three LED pads.
    l32r a2, .L_funcsel
    movi a3, OUT_SEL
    s32i a3, a2, 0
    s32i a3, a2, 4
    s32i a3, a2, 8

    l32r a2, .L_gpiobase

    # Enable the output drivers on R, G, B.
    movi a3, RGB_MASK
    s32i a3, a2, OFF_ENABLE1_W1TS

    # Initial state 0 = Red.
    movi a3, 0x08
    s32i a3, a2, OFF_OUT1_W1TS
    movi a3, 0x30
    s32i a3, a2, OFF_OUT1_W1TC

    retw
    .size xtensa_gpio_init, .-xtensa_gpio_init

# ---------------------------------------------------------------------------
# Debounced edge detect for one button, done inline.
#   \bitpos  bit position in GPIO_IN1
#   \off     byte offset of this button's block inside g_vars
#   \res     register that receives 1 on a settled press, else 0
# Uses: a3 = IN1 snapshot, a4 = CCOUNT snapshot, a5 = &g_vars
# Clobbers a6, a7, a8, a9
# ---------------------------------------------------------------------------
    .macro DEBOUNCE bitpos, off, res
    extui a6, a3, \bitpos, 1            # raw pin level
    movi  a7, 1
    sub   a7, a7, a6                    # pull-up: pressed reads low, so invert

    l32i  a8, a5, \off+4                # lastRead
    beq   a7, a8, 8f
    s32i  a7, a5, \off+4                # level moved, restart the window
    s32i  a4, a5, \off+8                # lastChange = now
8:
    l32i  a8, a5, \off+8                # lastChange
    sub   a8, a4, a8                    # elapsed cycles
    l32r  a9, .L_debounce
    movi  \res, 0
    bltu  a8, a9, 9f                    # still inside the bounce window
    l32i  a8, a5, \off                  # stable
    beq   a7, a8, 9f                    # nothing new
    s32i  a7, a5, \off                  # accept the new level
    mov   \res, a7                      # 1 only on the press edge
9:
    .endm

# ---------------------------------------------------------------------------
# void xtensa_gpio_poll(void)
# ---------------------------------------------------------------------------
    .align 4
    .global xtensa_gpio_poll
    .type   xtensa_gpio_poll, @function
xtensa_gpio_poll:
    entry a1, 32

    l32r a2, .L_gpiobase
    l32i a3, a2, OFF_IN1                # one snapshot of both buttons
    rsr.ccount a4                       # one timestamp for both
    l32r a5, .L_vars

    DEBOUNCE 15, 4,  a10                # forward  -> GPIO47
    DEBOUNCE 16, 16, a11                # backward -> GPIO48

    beq  a10, a11, .Ldone               # neither, or both: do nothing

    l32i a6, a5, 0                      # state
    beqz a10, .Lback
    addi a6, a6, 1                      # forward
    j    .Lwrap
.Lback:
    addi a6, a6, 3                      # backward, +3 == -1 mod 4
.Lwrap:
    movi a7, 3
    and  a6, a6, a7                     # mod 4
    s32i a6, a5, 0

    l32r a7, .L_colortab
    addx4 a7, a6, a7                    # &color_tab[state]
    l32i a8, a7, 0                      # bits that should be on

    movi a9, RGB_MASK
    xor  a9, a9, a8                     # bits that should be off

    s32i a9, a2, OFF_OUT1_W1TC
    s32i a8, a2, OFF_OUT1_W1TS

.Ldone:
    retw
    .size xtensa_gpio_poll, .-xtensa_gpio_poll
