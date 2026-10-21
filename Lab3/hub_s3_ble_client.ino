#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

constexpr uint8_t BUTTON_PIN=4;
constexpr uint8_t RGB_PIN=38;
constexpr uint32_t AUTO_LOCK_MS=30000, DEBOUNCE_MS=45, BLUE_FLASH_MS=250, SCAN_SECONDS=1;
constexpr char CODE[]="1234";        // must match what the user types on the keypad

#define SERVICE_UUID "a8f1c6e0-4180-4a5e-9b2d-0a4180000001"
#define KEY_UUID     "a8f1c6e0-4180-4a5e-9b2d-0a4180000002"
#define STATE_UUID   "a8f1c6e0-4180-4a5e-9b2d-0a4180000003"

struct __attribute__((packed)) StateMsg {uint8_t armed; uint8_t result;};

QueueHandle_t keyQueue;
BLEClient* client=nullptr;
BLERemoteCharacteristic* keyChr=nullptr;
BLERemoteCharacteristic* stateChr=nullptr;
BLEAdvertisedDevice* target=nullptr;
volatile bool doConnect=false;
volatile bool linked=false;
bool prevLinked=false;

bool armed=true;
uint32_t unlockedAt=0, flashUntil=0;
String entered="";
int stableButton=HIGH, rawButton=HIGH;
uint32_t changedAt=0;

void rgb(uint8_t r,uint8_t g,uint8_t b){neopixelWrite(RGB_PIN,r,g,b);}
void flashBlue(){flashUntil=millis()+BLUE_FLASH_MS;}
void showLed(){
  if((int32_t)(flashUntil-millis())>0) rgb(0,0,80);
  else if(armed) rgb(80,0,0);
  else rgb(0,80,0);
}

// Write state to the keypad (called from loop() only, never from a BLE callback).
void pushState(uint8_t result){
  if(!linked||!stateChr) return;
  StateMsg m{uint8_t(armed),result};
  stateChr->writeValue(reinterpret_cast<uint8_t*>(&m),sizeof(m),true);
}
void changeState(bool next,const char* source){
  if(armed==next) return;
  armed=next; entered="";
  if(!armed) unlockedAt=millis();
  Serial.printf("%s: %s\n",source,armed?"ARMED":"DISARMED");
  pushState(0);
}

// ---------- BLE callbacks (BLE task: enqueue / set flags only) ----------
// onNotify: keypad pushed a key
static void onNotify(BLERemoteCharacteristic*,uint8_t* data,size_t len,bool){
  if(len!=1||!keyQueue) return;
  char k=(char)data[0];
  if((k>='0'&&k<='9')||k=='B'||k=='E') xQueueSend(keyQueue,&k,0);
}
class ClientCB : public BLEClientCallbacks {
  void onConnect(BLEClient*) override {}
  void onDisconnect(BLEClient*) override { linked=false; }
};
class AdvCB : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) override {
    if(dev.haveServiceUUID()&&dev.isAdvertisingService(BLEUUID(SERVICE_UUID))){
      BLEDevice::getScan()->stop();
      delete target;
      target=new BLEAdvertisedDevice(dev);
      doConnect=true;
    }
  }
};

bool connectToKeypad(){
  if(!client){client=BLEDevice::createClient(); client->setClientCallbacks(new ClientCB());}
  if(!client->connect(target)){Serial.println("Connect failed");return false;}
  BLERemoteService* svc=client->getService(SERVICE_UUID);
  if(!svc){Serial.println("Service not found");client->disconnect();return false;}
  keyChr=svc->getCharacteristic(KEY_UUID);
  stateChr=svc->getCharacteristic(STATE_UUID);
  if(!keyChr||!stateChr||!keyChr->canNotify()){
    Serial.println("Characteristic missing");client->disconnect();return false;
  }
  keyChr->registerForNotify(onNotify);          // subscribe -> onNotify()
  entered="";
  linked=true;
  Serial.println("Keypad connected");
  pushState(0);                                 // sync current state to keypad
  return true;
}

void handleKey(char k){
  Serial.printf("Key '%c'\n",k);
  if(!armed){changeState(true,"Touch after unlock");return;}   // any key re-arms
  if(k>='0'&&k<='9'){ if(entered.length()<4) entered+=k; }
  else if(k=='B'){ if(entered.length()) entered.remove(entered.length()-1); }
  else if(k=='E'){
    bool ok=(entered==CODE);
    entered="";
    if(ok) changeState(false,"Correct passcode");
    else {Serial.println("Invalid passcode"); pushState(1);}
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  rgb(80,0,0);
  keyQueue=xQueueCreate(20,sizeof(char));
  if(!keyQueue){Serial.println("Queue allocation FAILED");while(true)delay(1000);}
  BLEDevice::init("ECE4180-Hub");
  BLEScan* scan=BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new AdvCB());
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(99);
  Serial.println("S3 hub ready: scanning for keypad; GPIO4 toggles arm/disarm");
}

void loop(){
  const uint32_t now=millis();

  // Debounced indoor pushbutton
  int sample=digitalRead(BUTTON_PIN);
  if(sample!=rawButton){rawButton=sample;changedAt=now;}
  if(now-changedAt>=DEBOUNCE_MS&&stableButton!=rawButton){
    stableButton=rawButton;
    if(stableButton==LOW) changeState(!armed,"Indoor button");
  }

  // Link management
  if(linked!=prevLinked){
    prevLinked=linked; flashBlue();
    Serial.println(linked?"Link up":"Keypad disconnected (hub keeps working)");
    if(!linked) entered="";
  }
  if(!linked){
    if(doConnect){ doConnect=false; connectToKeypad(); }
    else {
      BLEDevice::getScan()->start(SCAN_SECONDS,false);   // short blocking scan while offline
      BLEDevice::getScan()->clearResults();
    }
  }

  char k;
  while(xQueueReceive(keyQueue,&k,0)==pdTRUE) handleKey(k);

  if(!armed&&now-unlockedAt>=AUTO_LOCK_MS) changeState(true,"Auto-lock 30s");

  showLed();
  delay(5);
}
