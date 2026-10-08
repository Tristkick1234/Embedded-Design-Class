#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST7735.h>
// LCD pins
#define TFT_SCLK 6
#define TFT_MOSI 7
#define TFT_DC   1
#define TFT_CS   0
#define TFT_RST  3

// Nav switch pins (active LOW)
#define PIN_UP     23
#define PIN_DOWN   22
#define PIN_LEFT   21
#define PIN_RIGHT  20
#define PIN_CENTER 19

#define SCREEN_W 240
#define SCREEN_H 240
#define RADIUS   25
#define STEP     3

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);

const uint16_t colors[] = {
  ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE,
  ST77XX_YELLOW, ST77XX_CYAN, ST77XX_MAGENTA, ST77XX_WHITE
};
const int NUM_COLORS = sizeof(colors) / sizeof(colors[0]);

int ballX = SCREEN_W / 2;
int ballY = SCREEN_H / 2;
int colorIdx = 0;
bool lastCenter = false;

void drawBall(int x, int y, uint16_t c) {
  tft.fillCircle(x, y, RADIUS, c);
}

void setup() {
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_CENTER, INPUT_PULLUP);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS); 
  tft.init(240, 240, SPI_MODE0);
  tft.setSPISpeed(20000000);
  tft.setRotation(2);          //adjust 0-3 if the image is rotated
  tft.fillScreen(ST77XX_BLACK);

  drawBall(ballX, ballY, colors[colorIdx]);
}

void loop() {
  int newX = ballX;
  int newY = ballY;

  //Each direction is read independently, so pressing two at once moves diagonally
  if (digitalRead(PIN_LEFT)  == LOW) newX -= STEP;
  if (digitalRead(PIN_RIGHT) == LOW) newX += STEP;
  if (digitalRead(PIN_UP)    == LOW) newY -= STEP;
  if (digitalRead(PIN_DOWN)  == LOW) newY += STEP;

  //Keep the ball's edge on screen
  newX = constrain(newX, RADIUS, SCREEN_W - 1 - RADIUS);
  newY = constrain(newY, RADIUS, SCREEN_H - 1 - RADIUS);

  //Center press: change color on press only, not while held
  bool center = (digitalRead(PIN_CENTER) == LOW);
  bool colorChanged = false;
  if (center && !lastCenter) {
    colorIdx = (colorIdx + 1) % NUM_COLORS;
    colorChanged = true;
  }
  lastCenter = center;

  // Redraw only if something changed
  if (newX != ballX || newY != ballY || colorChanged) {
    drawBall(ballX, ballY, ST77XX_BLACK);     // erase old
    ballX = newX;
    ballY = newY;
    drawBall(ballX, ballY, colors[colorIdx]); // draw new
  }

  delay(15);
}
