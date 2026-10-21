
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MPR121.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>   // CCCD (needed for notify on Bluedroid; deprecated/harmless on newer cores)
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

constexpr uint8_t SDA_PIN=4, SCL_PIN=5;
constexpr uint8_t LCD_SCLK=6, LCD_MOSI=7, LCD_DC=1, LCD_CS=0, LCD_RST=3;
constexpr uint8_t RGB_PIN=8;
constexpr uint32_t BLUE_FLASH_MS=250, WRONG_MS=1500;

#define SERVICE_UUID "a8f1c6e0-4180-4a5e-9b2d-0a4180000001"
#define KEY_UUID     "a8f1c6e0-4180-4a5e-9b2d-0a4180000002"
#define STATE_UUID   "a8f1c6e0-4180-4a5e-9b2d-0a4180000003"

// Pads 0..9 digits, 10 = backspace, 11 = enter
const char KEYS[12]={'0','1','2','3','4','5','6','7','8','9','B','E'};

struct __attribute__((packed)) StateMsg {uint8_t armed; uint8_t result;};

QueueHandle_t stateQueue;
BLEServer* server=nullptr;
BLECharacteristic* keyChar=nullptr;
Adafruit_MPR121 cap;
Adafruit_ST7789 tft(&SPI,LCD_CS,LCD_DC,LCD_RST);

volatile bool bleConnected=false;
volatile bool restartAdv=false;
bool prevConnected=false;
bool keypadReady=false;
bool armed=true;             // safe default
bool redraw=true;
bool wrongShown=false;
uint32_t flashUntil=0, wrongUntil=0;
uint16_t previousTouched=0;
String entered="";

void rgb(uint8_t r,uint8_t g,uint8_t b){neopixelWrite(RGB_PIN,r,g,b);}
void flashBlue(){flashUntil=millis()+BLUE_FLASH_MS;}
void showLed(){
  if((int32_t)(flashUntil-millis())>0) rgb(0,0,80);
  else if(armed) rgb(80,0,0);
  else rgb(0,80,0);
}

// ---------- BLE callbacks (run in the BLE task: flag/queue only) ----------
class ServerCB : public BLEServerCallbacks {
  void onConnect(BLEServer*) override { bleConnected=true; }
  void onDisconnect(BLEServer*) override { bleConnected=false; restartAdv=true; }
};
class StateCB : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {        // hub wrote the lock state
    if(c->getLength()!=sizeof(StateMsg)) return;
    StateMsg m; memcpy(&m,c->getData(),sizeof(m));
    if(m.armed>1||m.result>1) return;
    xQueueSend(stateQueue,&m,0);
  }
};

void initBle(){
  BLEDevice::init("ECE4180-Keypad");
  server=BLEDevice::createServer();
  server->setCallbacks(new ServerCB());
  BLEService* svc=server->createService(SERVICE_UUID);

  keyChar=svc->createCharacteristic(KEY_UUID,BLECharacteristic::PROPERTY_NOTIFY);
  keyChar->addDescriptor(new BLE2902());

  BLECharacteristic* stateChar=svc->createCharacteristic(STATE_UUID,BLECharacteristic::PROPERTY_WRITE);
  stateChar->setCallbacks(new StateCB());

  svc->start();
  BLEAdvertising* adv=BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("BLE advertising as ECE4180-Keypad");
}

void sendKey(char k){
  uint8_t b=(uint8_t)k;
  keyChar->setValue(&b,1);
  keyChar->notify();
}

void handleState(const StateMsg& m){
  armed=m.armed;
  entered="";                         // every state message ends the current entry
  if(m.result==1){wrongShown=true; wrongUntil=millis()+WRONG_MS;}
  redraw=true;
  Serial.printf("Hub state -> %s%s\n",armed?"ARMED":"DISARMED",m.result==1?" (wrong code)":"");
}

void touchKey(char key){
  Serial.printf("MPR121 key %c\n",key);
  if(!bleConnected){Serial.println("No hub connected - key ignored");return;}
  if(armed){                          // local echo only while locked
    if(key>='0'&&key<='9'){ if(entered.length()<4) entered+=key; }
    else if(key=='B'){ if(entered.length()) entered.remove(entered.length()-1); }
    redraw=true;
  }
  sendKey(key);                       // hub decides everything else
}

void showScreen(){
  if(!redraw)return;
  redraw=false;
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(2);
  tft.setCursor(10,12);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("SECURITY SYSTEM");
  tft.setCursor(10,54);
  tft.setTextColor(armed?ST77XX_RED:ST77XX_GREEN);
  tft.setTextSize(3);
  tft.print(armed?"LOCKED":"UNLOCKED");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10,115);
  tft.print("Code: ");
  tft.print(entered);                 // shown for lab demo (not masked)
  if(wrongShown){
    tft.setCursor(10,140);
    tft.setTextColor(ST77XX_RED);
    tft.print("WRONG CODE");
    tft.setTextColor(ST77XX_WHITE);
  }
  tft.setTextSize(1);
  tft.setCursor(10,165);
  tft.print("Pads 0-9 = digits");
  tft.setCursor(10,179);
  tft.print("Pad 10 = BKSP, Pad 11 = ENTER");
  tft.setCursor(10,193);
  tft.print(bleConnected?"Hub linked (BLE)":"Hub offline - advertising");
  tft.setCursor(10,215);
  tft.print("Pad: ");tft.print(keypadReady?"READY":"NOT FOUND");
}

void setup(){
  Serial.begin(115200);
  delay(2000);
  Serial.println("1: boot");
  rgb(80,0,0);
  SPI.begin(LCD_SCLK,-1,LCD_MOSI,LCD_CS);
  tft.init(240,240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  Serial.println("2: tft ok");
  Wire.begin(SDA_PIN,SCL_PIN);
  keypadReady=cap.begin(0x5A);
  if(keypadReady) cap.setAutoconfig(true);
  Serial.println(keypadReady?"3: MPR121 ok":"3: MPR121 NOT FOUND");
  stateQueue=xQueueCreate(8,sizeof(StateMsg));
  if(!stateQueue){Serial.println("Queue allocation FAILED");while(true)delay(1000);}
  initBle();
  Serial.println("4: ble ok");
  showScreen();
  Serial.println("5: setup done");
}

void loop(){
  uint32_t now=millis();

  StateMsg m;
  while(xQueueReceive(stateQueue,&m,0)==pdTRUE) handleState(m);

  if(bleConnected!=prevConnected){
    prevConnected=bleConnected;
    entered=""; wrongShown=false; redraw=true; flashBlue();
    if(!bleConnected) armed=true;     // safe display until hub reconnects and re-sends state
    Serial.println(bleConnected?"Hub connected":"Hub disconnected");
  }
  if(restartAdv&&!bleConnected){
    restartAdv=false;
    delay(100);
    BLEDevice::startAdvertising();
    Serial.println("Advertising restarted");
  }
  if(wrongShown&&(int32_t)(now-wrongUntil)>=0){wrongShown=false;redraw=true;}

  if(keypadReady){
    uint16_t current=cap.touched();
    uint16_t newTouches=current&~previousTouched;
    uint16_t released=previousTouched&~current;
    previousTouched=current;
    for(uint8_t i=0;i<12;i++){
      if(newTouches&(1U<<i)) Serial.printf("TOUCH PRESSED: pad %u, key '%c'\n",i,KEYS[i]);
      if(released&(1U<<i))   Serial.printf("TOUCH RELEASED: pad %u, key '%c'\n",i,KEYS[i]);
    }
    for(uint8_t i=0;i<12;i++){
      if(newTouches&(1U<<i)){touchKey(KEYS[i]);break;}   // first new key per scan
    }
  }
  showLed();
  showScreen();
  delay(15);
}
