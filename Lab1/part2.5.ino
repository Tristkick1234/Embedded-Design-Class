setup_hardware:
    # 1. Load Base Addresses using LUI (Load Upper Immediate)
    # LUI shifts the value left by 12 bits, so 0x60090 becomes 0x60090000
    lui t0, 0x60090          # t0 = 0x60090000 (IO MUX Base)
    lui t1, 0x60091          # t1 = 0x60091000 (GPIO Matrix Base)

    # 2. Clear IO_MUX_GPIO11 bits (7, 8, 9, 12, 13, 14) -> Mask: 0x7380
    lw  t2, 0x30(t0)         # Load current value of MY_IO_MUX_GPIO11 (Base + 0x30 offset)
    li  t3, 0x7380           # Load the clear mask into t3
    not t3, t3               # Bitwise NOT the mask (replaces the ~ operator)
    and t2, t2, t3           # t2 = t2 AND NOT mask
    sw  t2, 0x30(t0)         # Store the cleared value back to memory

    # 3. Set IO_MUX_GPIO11 bits (7, 9, 12) -> Mask: 0x1280
    lw  t2, 0x30(t0)         # Load the newly cleared value
    li  t3, 0x1280           # Load the set mask
    or  t2, t2, t3           # Bitwise OR to set the bits
    sw  t2, 0x30(t0)         # Store it back

    # 4. Set IO_MUX_GPIO7 bit 12 -> Mask: 0x1000 (1 << 12)
    lw  t2, 0x20(t0)         # Load MY_IO_MUX_GPIO7 (Base + 0x20 offset)
    li  t3, 0x1000           # Load the mask
    or  t2, t2, t3           # Bitwise OR to set bit 12
    sw  t2, 0x20(t0)         # Store it back

    # 5. Enable GPIO 7 Output (*MY_GPIO_OUTPUT_EN = 128)
    li  t2, 128              # Load 128 (1 << 7) into t2
    sw  t2, 0x24(t1)         # Store directly to MY_GPIO_OUTPUT_EN (GPIO Base + 0x24)

loop_hardware:
    # 6. Read MY_GPIO_INPUT
    lw  t2, 0x3C(t1)         # Load MY_GPIO_INPUT (GPIO Base + 0x3C)
    li  t3, 2048             # Load 2048 (1 << 11) to check bit 11
    and t2, t2, t3           # t2 = t2 AND 2048 (isolates bit 11)

    # 7. Check if HIGH or LOW (Branch if Equal to Zero)
    beqz t2, button_is_low   # If t2 == 0, jump to button_is_low label

button_is_high:
    # 8. Turn LED ON (*MY_GPIO_OUTPUT_SET = 128)
    li  t4, 128     # Load 128 (1 << 7)
    sw  t4, 0x08(t1)     # Store to MY_GPIO_OUTPUT_SET (GPIO Base + 0x08)
    j   end_of_check      # Jump over the LOW condition

button_is_low:
    # 9. Turn LED OFF (*MY_GPIO_OUTPUT_CLR = 128)
    li  t4, 128   # Load 128 (1 << 7)
    sw  t4, 0x0C(t1)   # Store to MY_GPIO_OUTPUT_CLR (GPIO Base + 0x0C)

end_of_check:
    jal loop_hardware;
