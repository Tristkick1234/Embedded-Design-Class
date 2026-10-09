/*
 * ESP32-C6 + ST7789 240x240 LCD: nav-switch ball, NO LCD library.
 *
 *  - SPI: GP-SPI2 (FSPI) driven by direct register access via volatile pointers (TRM Ch.28)
 *  - CS / DC / RST: raw GPIO registers via volatile pointers (TRM Ch.7)
 *  - Nav switch: Arduino pinMode/digitalRead (allowed by the assignment)
 *
 * Wiring
 *   LCD SCL -> GPIO6   (FSPICLK)      LCD CS  -> GPIO0
 *   LCD SDA -> GPIO7   (FSPID/MOSI)   LCD DC  -> GPIO1
 *   LCD RES -> GPIO3                  LCD BLK -> 3V3,  VCC -> 3V3,  GND -> GND
 *   Nav: UP 23, DOWN 22, LEFT 21, RIGHT 20, CENTER 19, COMMON -> GND
 */
#include <Arduino.h>

// ------------------------------------------------------------------
// Pins
// ------------------------------------------------------------------
#define PIN_SCLK 6
#define PIN_MOSI 7
#define PIN_CS   0
#define PIN_DC   1
#define PIN_RST  3

#define PIN_UP     23
#define PIN_DOWN   22
#define PIN_LEFT   21
#define PIN_RIGHT  20
#define PIN_CENTER 19

// ------------------------------------------------------------------
// Display / ball parameters
// ------------------------------------------------------------------
#define SCREEN_W 240
#define SCREEN_H 240
#define RADIUS   10
#define STEP     3

// Panel RAM offset. 0,0 matches MADCTL=0x00 on most 240x240 modules.
// If the image is shifted or cut off, try LCD_Y_OFFSET 80 (or flip MADCTL).
#define LCD_X_OFFSET 0
#define LCD_Y_OFFSET 0
#define LCD_MADCTL   0x00   // RGB order, no mirroring. 0x08 swaps R/B if colors are wrong
#define LCD_INVERT   1      // most 240x240 ST7789 modules need INVON

// SPI clock = 80 MHz / LCD_SPI_DIV  (4 -> 20 MHz, 2 -> 40 MHz)
#define LCD_SPI_DIV 4

// ------------------------------------------------------------------
// Registers as explicit pointers (addresses from TRM Table 5.3-2 + register summaries)
// Use:  *PTR = value;   value = *PTR;   PTR[n] for register arrays
// ------------------------------------------------------------------

// --- GPIO matrix (base 0x60091000) ---
static volatile uint32_t *const reg_gpio_out_w1ts        = (volatile uint32_t *)0x60091008;
static volatile uint32_t *const reg_gpio_out_w1tc        = (volatile uint32_t *)0x6009100C;
static volatile uint32_t *const reg_gpio_enable_w1ts     = (volatile uint32_t *)0x60091024;
// GPIO_FUNCn_OUT_SEL_CFG_REG = 0x60091554 + 4*n  -> index with [n]
//   [7:0] OUT_SEL, bit 9 OEN_SEL
static volatile uint32_t *const reg_gpio_func_out_sel_cfg = (volatile uint32_t *)0x60091554;

// --- IO MUX (base 0x60090000) ---
// IO_MUX_GPIOn_REG = 0x60090004 + 4*n  -> index with [n];  [14:12] MCU_SEL
static volatile uint32_t *const reg_io_mux_gpio          = (volatile uint32_t *)0x60090004;

// --- PCR (base 0x60096000): SPI2 clock/reset ---
static volatile uint32_t *const reg_pcr_spi2_conf        = (volatile uint32_t *)0x600960C0; // bit0 CLK_EN, bit1 RST_EN
static volatile uint32_t *const reg_pcr_spi2_clkm_conf   = (volatile uint32_t *)0x600960C4; // [21:20] SEL, bit22 EN

// --- GP-SPI2 (base 0x60081000) ---
static volatile uint32_t *const reg_spi_cmd      = (volatile uint32_t *)0x60081000;
static volatile uint32_t *const reg_spi_ctrl     = (volatile uint32_t *)0x60081008;
static volatile uint32_t *const reg_spi_clock    = (volatile uint32_t *)0x6008100C;
static volatile uint32_t *const reg_spi_user     = (volatile uint32_t *)0x60081010;
static volatile uint32_t *const reg_spi_ms_dlen  = (volatile uint32_t *)0x6008101C;
static volatile uint32_t *const reg_spi_misc     = (volatile uint32_t *)0x60081020;
static volatile uint32_t *const reg_spi_dma_conf = (volatile uint32_t *)0x60081030;
static volatile uint32_t *const reg_spi_w        = (volatile uint32_t *)0x60081098;  // W0..W15 -> reg_spi_w[0..15]
static volatile uint32_t *const reg_spi_slave    = (volatile uint32_t *)0x600810E0;
static volatile uint32_t *const reg_spi_clk_gate = (volatile uint32_t *)0x600810E8;

// Signal indices (Table 7.11-1)
static const uint32_t k_sig_fspiclk_out = 63;
static const uint32_t k_sig_fspid_out   = 65;
static const uint32_t k_sig_simple_gpio = 0x80;   // use GPIO_OUT_REG / GPIO_ENABLE_REG

// Bit masks
static const uint32_t k_spi_update        = 0x00800000;  // reg_spi_cmd
static const uint32_t k_spi_usr           = 0x01000000;  // reg_spi_cmd
static const uint32_t k_spi_usr_mosi      = 0x08000000;  // reg_spi_user
static const uint32_t k_spi_rd_bit_order  = 0x02000000;  // reg_spi_ctrl
static const uint32_t k_spi_wr_bit_order  = 0x04000000;  // reg_spi_ctrl
static const uint32_t k_spi_ck_idle_edge  = 0x20000000;  // reg_spi_misc
static const uint32_t k_spi_dma_afifo_rst = 0x80000000;  // reg_spi_dma_conf
static const uint32_t k_spi_buf_afifo_rst = 0x40000000;
static const uint32_t k_spi_rx_afifo_rst  = 0x20000000;

// ------------------------------------------------------------------
// Raw GPIO helpers
// ------------------------------------------------------------------
static inline void gpio_hi(int n) { *reg_gpio_out_w1ts = 1 << n; }
static inline void gpio_lo(int n) { *reg_gpio_out_w1tc = 1 << n; }

static void iomux_select_gpio_function(int n) {
  uint32_t v = reg_io_mux_gpio[n];
  v &= ~(0x7 << 12);
  v |= (1 << 12);                       // MCU_SEL = 1 -> GPIO function
  reg_io_mux_gpio[n] = v;
}

static void gpio_raw_output(int n) {      // plain output driven by GPIO_OUT_REG
  iomux_select_gpio_function(n);
  reg_gpio_func_out_sel_cfg[n] = k_sig_simple_gpio | (1 << 9);  // OEN from GPIO_ENABLE_REG
  *reg_gpio_enable_w1ts = 1 << n;
}

static void gpio_route_peripheral_out(int n, uint32_t sig) {   // GPIO matrix -> pin
  iomux_select_gpio_function(n);
  reg_gpio_func_out_sel_cfg[n] = sig | (1 << 9);
  *reg_gpio_enable_w1ts = 1 << n;
}

// ------------------------------------------------------------------
// SPI master (CPU-controlled, half-duplex write, 1-bit, mode 0, MSB first)
// ------------------------------------------------------------------
static void spi_init() {
  // 1) Clocks: enable APB clock, pulse module reset, pick PLL_F80M as function clock
  *reg_pcr_spi2_conf |= (1 << 0);                      // PCR_SPI2_CLK_EN
  *reg_pcr_spi2_conf |= (1 << 1);                      // PCR_SPI2_RST_EN = 1
  *reg_pcr_spi2_conf &= ~(1 << 1);                     // release reset
  uint32_t k = *reg_pcr_spi2_clkm_conf;
  k &= ~(0x3 << 20);
  k |= (1 << 20) | (1 << 22);                    // CLKM_SEL=1 (PLL_F80M), CLKM_EN=1
  *reg_pcr_spi2_clkm_conf = k;

  // SPI_CLK_EN is bit0 per TRM. Bits 1-2 are "reserved" in this TRM revision;
  // ESP-IDF's driver sets them too (master clock active/select), so set them: harmless if reserved.
  *reg_spi_clk_gate = (1 << 0) | (1 << 1) | (1 << 2);

  // 2) Master mode
  *reg_spi_slave = 0;                         // SPI_SLAVE_MODE = 0

  // 3) Only the MOSI (DOUT) state; CMD/ADDR/DUMMY/MISO off, no CS setup/hold
  //    (USR_COMMAND resets to 1, CS_SETUP/CS_HOLD reset to 1, so write the whole reg)
  //    CK_OUT_EDGE (bit9) = 0, DOUTDIN (bit0) = 0 -> half-duplex
  *reg_spi_user = k_spi_usr_mosi;

  // 4) Mode 0: CK_IDLE_EDGE = 0 and CK_OUT_EDGE = 0 (Table 28.7-1)
  *reg_spi_misc &= ~k_spi_ck_idle_edge;

  // 5) MSB first (0 = MSB first)
  *reg_spi_ctrl &= ~(k_spi_wr_bit_order | k_spi_rd_bit_order);

  // 6) CPU-controlled transfers: DMA off
  *reg_spi_dma_conf = 0;

  // 7) Clock divider: f = f_clk_spi_mst / ((N+1)*(PRE+1)),  CLK_EQU_SYSCLK = 0
  const uint32_t N = LCD_SPI_DIV - 1;
  const uint32_t H = (LCD_SPI_DIV / 2) - 1;              // floor((N+1)/2 - 1)
  const uint32_t L = N;                              // must equal N in master mode
  *reg_spi_clock = (N << 12) | (H << 6) | L;   // PRE = 0, EQU_SYSCLK = 0

  // 8) Route FSPICLK / FSPID to the pins through the GPIO matrix
  gpio_route_peripheral_out(PIN_SCLK, k_sig_fspiclk_out);
  gpio_route_peripheral_out(PIN_MOSI, k_sig_fspid_out);
}

static inline void spi_wait_clear(volatile uint32_t *reg, uint32_t mask) {
  uint32_t guard = 2000000;
  while ((*reg & mask) && --guard) { }
}

// Send up to 64 bytes in one hardware transaction
static void spi_write_chunk(const uint8_t *buf, size_t n) {
  for (size_t i = 0; i < n; i += 4) {                // byte0 = W0[7:0], byte1 = W0[15:8], ...
    uint32_t v = 0;
    for (size_t j = 0; j < 4 && (i + j) < n; j++) v |= (uint32_t)buf[i + j] << (8 * j);
    reg_spi_w[i / 4] = v;
  }
  *reg_spi_ms_dlen = (uint32_t)(n * 8 - 1);    // bits - 1

  *reg_spi_dma_conf = k_spi_dma_afifo_rst | k_spi_buf_afifo_rst | k_spi_rx_afifo_rst;
  *reg_spi_dma_conf = 0;

  *reg_spi_cmd = k_spi_update;                             // sync regs into SPI clock domain
  spi_wait_clear(reg_spi_cmd, k_spi_update);
  *reg_spi_cmd = k_spi_usr;                                // start transfer
  spi_wait_clear(reg_spi_cmd, k_spi_usr);                  // hardware clears USR when done
}

static void spi_write(const uint8_t *buf, size_t len) {
  while (len) {
    size_t n = (len > 64) ? 64 : len;
    spi_write_chunk(buf, n);
    buf += n;
    len -= n;
  }
}

// ------------------------------------------------------------------
// ST7789 (4-line SPI: DC low = command, DC high = data; CS active low)
// ------------------------------------------------------------------
#define ST_SWRESET 0x01
#define ST_SLPOUT  0x11
#define ST_NORON   0x13
#define ST_INVON   0x21
#define ST_DISPON  0x29
#define ST_CASET   0x2A
#define ST_RASET   0x2B
#define ST_RAMWR   0x2C
#define ST_MADCTL  0x36
#define ST_COLMOD  0x3A

static inline void cs_low()  { gpio_lo(PIN_CS); }
static inline void cs_high() { gpio_hi(PIN_CS); }
static inline void dc_cmd()  { gpio_lo(PIN_DC); }
static inline void dc_data() { gpio_hi(PIN_DC); }

static void lcd_cmd(uint8_t c, const uint8_t *d = nullptr, size_t n = 0) {
  cs_low();
  dc_cmd();
  spi_write(&c, 1);
  if (n) {
    dc_data();
    spi_write(d, n);
  }
  cs_high();
}

static void lcd_set_window(int x0, int y0, int x1, int y1) {
  x0 += LCD_X_OFFSET; x1 += LCD_X_OFFSET;
  y0 += LCD_Y_OFFSET; y1 += LCD_Y_OFFSET;
  uint8_t cx[4] = { (uint8_t)(x0 >> 8), (uint8_t)x0, (uint8_t)(x1 >> 8), (uint8_t)x1 };
  uint8_t ry[4] = { (uint8_t)(y0 >> 8), (uint8_t)y0, (uint8_t)(y1 >> 8), (uint8_t)y1 };
  lcd_cmd(ST_CASET, cx, 4);
  lcd_cmd(ST_RASET, ry, 4);
}

static void lcd_ramwr_begin() {     // keep CS low from RAMWR through the last pixel
  uint8_t c = ST_RAMWR;
  cs_low();
  dc_cmd();
  spi_write(&c, 1);
  dc_data();
}
static void lcd_ramwr_end() { cs_high(); }

static void lcd_init() {
  gpio_raw_output(PIN_CS);
  gpio_raw_output(PIN_DC);
  gpio_raw_output(PIN_RST);
  cs_high(); dc_data(); gpio_hi(PIN_RST);

  // Hardware reset
  gpio_lo(PIN_RST); delay(20);
  gpio_hi(PIN_RST); delay(120);

  lcd_cmd(ST_SWRESET);            delay(150);
  lcd_cmd(ST_SLPOUT);             delay(120);

  uint8_t colmod = 0x55;          // 16 bit/pixel (RGB565)
  lcd_cmd(ST_COLMOD, &colmod, 1); delay(10);
  uint8_t madctl = LCD_MADCTL;
  lcd_cmd(ST_MADCTL, &madctl, 1);
#if LCD_INVERT
  lcd_cmd(ST_INVON);
#endif
  lcd_cmd(ST_NORON);              delay(10);
  lcd_cmd(ST_DISPON);             delay(120);
}

static void lcd_fill_black() {
  static const uint8_t zeros[64] = { 0 };
  lcd_set_window(0, 0, SCREEN_W - 1, SCREEN_H - 1);
  lcd_ramwr_begin();
  for (int i = 0; i < (SCREEN_W * SCREEN_H * 2) / 64; i++) spi_write(zeros, 64);
  lcd_ramwr_end();
}

// Redraw only the union of the old and new ball boxes in ONE window write:
// each pixel is ball color if inside the new circle, else black. No flicker.
static void draw_ball(int oldX, int oldY, int newX, int newY, uint16_t color) {
  int x0 = (oldX < newX ? oldX : newX) - RADIUS;
  int x1 = (oldX > newX ? oldX : newX) + RADIUS;
  int y0 = (oldY < newY ? oldY : newY) - RADIUS;
  int y1 = (oldY > newY ? oldY : newY) + RADIUS;

  lcd_set_window(x0, y0, x1, y1);
  lcd_ramwr_begin();

  uint8_t buf[64];
  size_t n = 0;
  for (int y = y0; y <= y1; y++) {
    for (int x = x0; x <= x1; x++) {
      int dx = x - newX, dy = y - newY;
      uint16_t c = (dx * dx + dy * dy <= RADIUS * RADIUS) ? color : 0x0000;
      buf[n++] = (uint8_t)(c >> 8);                  // RGB565, high byte first
      buf[n++] = (uint8_t)(c & 0xFF);
      if (n == sizeof(buf)) { spi_write(buf, n); n = 0; }
    }
  }
  if (n) spi_write(buf, n);
  lcd_ramwr_end();
}

// ------------------------------------------------------------------
// Application
// ------------------------------------------------------------------
static const uint16_t colors[] = {
  0xF800, // red
  0x07E0, // green
  0x001F, // blue
  0xFFE0, // yellow
  0x07FF, // cyan
  0xF81F, // magenta
  0xFFFF  // white
};
static const int NUM_COLORS = sizeof(colors) / sizeof(colors[0]);

static int  ballX = SCREEN_W / 2;
static int  ballY = SCREEN_H / 2;
static int  colorIdx = 0;
static bool lastCenter = false;

void setup() {
  pinMode(PIN_UP,     INPUT_PULLUP);
  pinMode(PIN_DOWN,   INPUT_PULLUP);
  pinMode(PIN_LEFT,   INPUT_PULLUP);
  pinMode(PIN_RIGHT,  INPUT_PULLUP);
  pinMode(PIN_CENTER, INPUT_PULLUP);

  spi_init();
  lcd_init();
  lcd_fill_black();
  draw_ball(ballX, ballY, ballX, ballY, colors[colorIdx]);
}


void loop() {
  int newX = ballX;
  int newY = ballY;

  // Each direction is read independently, so pressing two at once moves diagonally
  if (digitalRead(PIN_LEFT)  == LOW) newX -= STEP;
  if (digitalRead(PIN_RIGHT) == LOW) newX += STEP;
  if (digitalRead(PIN_UP)    == LOW) newY -= STEP;
  if (digitalRead(PIN_DOWN)  == LOW) newY += STEP;

  // Keep the ball's edge on screen
  newX = constrain(newX, RADIUS, SCREEN_W - 1 - RADIUS);
  newY = constrain(newY, RADIUS, SCREEN_H - 1 - RADIUS);

  // Center press: change color on press only, not while held
  bool center = (digitalRead(PIN_CENTER) == LOW);
  bool colorChanged = false;
  if (center && !lastCenter) {
    colorIdx = (colorIdx + 1) % NUM_COLORS;
    colorChanged = true;
  }
  lastCenter = center;

  // Redraw only if something changed
  if (newX != ballX || newY != ballY || colorChanged) {
    draw_ball(ballX, ballY, newX, newY, colors[colorIdx]);  // erase old + draw new in one pass
    ballX = newX;
    ballY = newY;
  }

  delay(15);
}
