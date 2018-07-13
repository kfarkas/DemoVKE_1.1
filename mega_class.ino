#include <avr/sleep.h>
#include <avr/power.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define INT_PIN 2
#define TANK_PIN 6
#define LINE_PIN 8
#define IR_PIN_FRONT A0
#define IR_PIN_END A1
#define CAR_CLOSE 600
#define CAR_AWAY  450
#define LEDS_TANK_NUM 6
#define LEDS_SIDE_NUM 60
#define LAP_NUM 6
#define ERR_RESET 15
#define TANK_COLOR strip.Color(165,42,0)
#define SIDE_COLOR strip2.Color(0,165,0)
#define ERROR_COLOR strip2.Color(165,0,0)
#define OFF_COLOR strip2.Color(0,0,0)
#define SIDE_OFF_SPEED 5
#define SIDE_ON_SPEED 85 //500
#define TANK_FLASH_SPEED 150
#define TANK_ERROR_FLASH_SPEED 500
#define TANK_ERROR_DURATION 10
#define BELT_FLASH_SPEED 500


#define ERROR_DELAY 1000

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS_TANK_NUM, TANK_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(LEDS_SIDE_NUM, LINE_PIN, NEO_GRB + NEO_KHZ800);

//======================================================================================================
void turnoffWipe_side(uint32_t c, uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void restart_mega(void);

class IRSensor
{
  char* pin;
  uint16_t carClose;
  uint16_t carAway;
  uint32_t updateInterval;
  uint32_t lastUpdate;
  
  public: 
  IRSensor(char* irPin, uint16_t distanceClose, uint16_t distanceAway, uint32_t updateInterval)
  {
    pin = irPin;
    pinMode(pin, INPUT);
    carClose = distanceClose;
    carAway = distanceAway;
    lastUpdate = 0;
  }
  uint8_t Update()
  {
    uint16_t irValue = 0;
    if(millis() - lastUpdate > updateInterval)
    {
      irValue = analogRead(pin);
      //Serial.println(irValue);
      lastUpdate = millis();
      if(irValue > carClose)
      {
        return 1;
      }
      if(irValue < carAway)
      {
        return 0;
      }
    }
  }
};

class SerialInfo
{
  uint8_t index;
  uint32_t lastUpdate;
  uint32_t updateInterval;
  
  public:
  SerialInfo(uint32_t updateInt)
  {
    index = 0;
    lastUpdate = 0;
    updateInterval = updateInt;
  }
  void Update(char* message)
  { 
    if(millis() - lastUpdate > updateInterval)
    {
      index = 0;
      char incomingByte;
      lastUpdate = millis();
      if(Serial.available())
      {
        while(Serial.available())
        {
          incomingByte = Serial.read();
          message[index] = incomingByte;
          index++;
        }
      }
      else
      {
        message[0] = '\0';
      }
      if(message[0] != '\0')
      {
        message[index] = '\0';
        Serial.flush();
      }
    }
  }
};

//class SideLeds
//{
//  uint8_t toggle;
//  unsigned long long lastUpdate;
//  uint32_t lastUpdateOff;
//  uint32_t updateInterval;
//  
//  public:
//  SideLeds(uint32_t ledSpeed)
//  {
//    updateInterval = ledSpeed;
//    lastUpdate = 0;
//    lastUpdateOff = 0;
//    toggle = 0;
//    turnoffWipe_side(strip2.Color(0,0,0), 10);
//    strip2.show();
//  }
//  void Update(char* task, uint8_t i)
//  {
//    if(millis() - lastUpdate > updateInterval)
//    {
//      Serial.println("TURN_ON");
//      //Serial.println(task);
//      //Serial.println(sizeof("turnOn"));
//      lastUpdate = millis();
//      Serial.println("TURN_ON_2");
//      //task = "turnOn";
//      //if(strncmp(task,"turnOn",7) == 0)
//      if(task == "turnOn")
//      {
//        Serial.println("TURN_ON_3");
//        //turnOn(i);
//      }
//      else
//      {
//        Serial.println("else");
//      }
//    }
//    if(millis() - lastUpdate > ERROR_DELAY)
//    {
//      //lastUpdate = millis(); 
//      if(task == "errorFlash")
//      {
//        errorFlash(i);
//      }
//    }
//    if(millis() - lastUpdateOff > SIDE_OFF_SPEED)
//    {
//      //lastUpdateOff = millis();
//      if(task == "turnOff")
//      {
//        turnOff(i);
//      }
//    }
//  }
//  void turnOn(uint8_t i)
//  {
//    strip2.setPixelColor(i,SIDE_COLOR);
//    strip2.show();
//  }
//  void turnOff(uint8_t i)
//  {
//    strip2.setPixelColor(i,OFF_COLOR);
//    strip2.show();
//  }
//  void errorFlash(uint8_t i)
//  {
//    toggle != toggle;
//    for(uint8_t k = 0;k <= i;k++)
//    {
//      strip2.setPixelColor(i , toggle ? ERROR_COLOR : OFF_COLOR);
//    }
//    strip2.show();
//  }
//  void turnoffWipe_side(uint32_t c, uint8_t wait)
//  {
//    for(uint8_t i = LEDS_SIDE_NUM; i > 0; i--)
//    {
//      strip2.setPixelColor(i,c);
//      strip2.show();
//      delay(wait);
//    }
//    strip2.setPixelColor(0,c);
//    strip2.show();
//  }
//};

class TankLeds 
{
  uint8_t lap_count;
  uint8_t toggle;
  uint32_t lastUpdate;
  uint32_t lastUpdateError;
  uint32_t updateInterval;  
  
  public:
  TankLeds(uint32_t ledSpeed)
  {
    lap_count = LEDS_TANK_NUM - 1;
    toggle = 0;
    lastUpdate = 0;
    lastUpdateError = 0;
    updateInterval = ledSpeed;
    colorWipe(TANK_COLOR, 20);
    strip.show();
  }
  void turnOff()
  {
    strip.setPixelColor(lap_count,strip.Color(0,0,0));
    strip.show();
    lap_count--;
  }
  void errorFlash()
  {
    //if(millis() - lastUpdate > ERROR_DELAY)
    //{
    //  lastUpdateError = millis();
      toggle != toggle;
      strip.setPixelColor(0, toggle? strip.Color(160,0,0) : strip.Color(0,0,0));
      strip.show();
    //}
  }
  void flash()
  {
    if(millis() - lastUpdate > updateInterval)
    {
      lastUpdate = millis();
      toggle != toggle;
      strip.setPixelColor(0, toggle? TANK_COLOR : strip.Color(0,0,0));
      strip.show();
    }
  }
  void colorWipe(uint32_t c, uint8_t wait) 
  {
    for (uint8_t i = 0; i < LEDS_TANK_NUM; i++)
    {
      strip.setPixelColor(i, c);
      //delay(wait);
    }
       strip.show();
  }
  
};

IRSensor irFront(IR_PIN_FRONT,  CAR_CLOSE,  CAR_AWAY, 500);
IRSensor irEnd(IR_PIN_END,   CAR_CLOSE,  CAR_AWAY, 500);
SerialInfo serialData(1000);

uint8_t startWipe;
uint8_t offWipe;
uint8_t turnOnLeds;
uint8_t tankError;
uint8_t toggle;
uint8_t lapCount;
uint8_t tankErrorDuration;
uint8_t beltErrorReset;
uint8_t ledsAreOn;

uint8_t gwError;
uint8_t gwPowerError;
uint8_t plcError;

uint32_t millisLedOn;
uint32_t millisLedOff;
uint32_t millisTankFlash;
uint32_t millisTankErrorFlash;
uint32_t millisBeltErrorFlash;

String outputString;
uint8_t error;
uint8_t ledNumOn;
uint8_t ledNumOff;
char* inputCh;
void setup() 
{
  strip.begin();
  strip2.begin();
  colorWipe(TANK_COLOR, 20);
  strip.show();
  turnoffWipe_side(strip2.Color(0,0,0), 10);
  strip2.show();
  startWipe = 0;
  tankErrorDuration = TANK_ERROR_DURATION;
  offWipe = 0;
  tankError = 0;
  turnOnLeds = 0;
  beltErrorReset = 0;
  toggle = 0;
  outputString = "";
  error &= 0x00;
  ledNumOn = 0;
  ledNumOff = 0;
  ledsAreOn = 0;
  inputCh = calloc(30,sizeof(char));
  Serial.begin(115200);
  //Serial.println("START");  
  millisLedOn = 0;
  millisLedOff = 0;
  millisTankFlash = 0;
  millisTankErrorFlash = 0;
  millisBeltErrorFlash = 0;
  lapCount = 0;
  gwError = 0;
  gwPowerError = 0;
  plcError = 0;
  Serial.flush();
  delay(2000);
}

void loop() 
{
//=================================SERIAL================================
  serialData.Update(inputCh);
  //Serial.print(inputCh);
  error = checkString(inputCh, error);
  if(inputCh[0] !=  '\0')
  {
    //Serial.println(inputCh);   
    inputCh[0] = '\0';
  }
  
  //comm_error & comm_power_error !
  if(!gwError && !gwPowerError && !plcError)
  {
    if(startWipe == 0)
    {
      startWipe = irFront.Update();
      if(startWipe)
      {
        turnOnLeds = 1;
        offWipe = 0;
        //outputString = "positionManagement_0 ";
        //sendSerial(outputString);
        
      }
    }
    if(offWipe == 0)
    {
      offWipe = irEnd.Update();
      if(offWipe)
      {
        startWipe = 0;
        //outputString = "positionManagement_1 ";
        //sendSerial(outputString);
      }
    }
  }
  if(ledNumOn < LEDS_SIDE_NUM && turnOnLeds && lapCount < 6)
  {
    if((millis() - millisLedOn > SIDE_ON_SPEED) && (plcError == 0) )
    {
      millisLedOn = millis();
      strip2.setPixelColor(ledNumOn,SIDE_COLOR);
      strip2.show();
      if(millis() - millisTankFlash > TANK_FLASH_SPEED)
      {
        millisTankFlash = millis();
        toggle = toggle? 0 : 1;
        strip.setPixelColor(lapCount, toggle? TANK_COLOR : strip.Color(0,0,0));
        strip.show();
      }
      //ledAllOn
      if(ledNumOn == (LEDS_SIDE_NUM - 1))
      {
        turnOnLeds = 0;
        ledNumOff = 0;
        ledsAreOn = 1;
        strip.setPixelColor(lapCount, TANK_COLOR);
        strip.show();
      }
      ledNumOn++;
    }
    //beltPLCERROR + ledTurnOn
    if(millis()- millisBeltErrorFlash > BELT_FLASH_SPEED)
    {
      millisBeltErrorFlash = millis();
      beltError(1);
    }
  }
  //beltPLCERROR + ledsAreOn //beltPLCERROR + ledsAreOff
  if((millis() - millisBeltErrorFlash > BELT_FLASH_SPEED) && (ledNumOn == 0 || ledNumOn == LEDS_SIDE_NUM))
  {
    millisBeltErrorFlash = millis();
    beltError(0);
  }
  
  
  
  if(ledNumOn == 60 && !turnOnLeds && offWipe && lapCount < 6)
  {
    if(millis()- millisLedOff > SIDE_OFF_SPEED)
    {
      millisLedOff = millis();
      strip2.setPixelColor(ledNumOff,OFF_COLOR);
      strip2.show();
      if(ledNumOff == LEDS_SIDE_NUM - 1)
      {
        strip.setPixelColor(lapCount, strip.Color(0,0,0));
        strip.show();
        //startWipe = 0;
        ledNumOn = 0;
        ledsAreOn = 0;
        lapCount++;
      }
      ledNumOff++;
    }
  }
  if(lapCount == 6)
  {
    //start tank error
    if(millis() - millisTankErrorFlash > TANK_ERROR_FLASH_SPEED)
    {
      millisTankErrorFlash = millis();
      toggle = toggle? 0 : 1;
      strip.setPixelColor(5, toggle? strip.Color(165,0,0) : strip.Color(0,0,0));
      strip.show();
      if(tankErrorDuration == 10)
      {
        sendSerial("no_liquid_error");
      }
      tankErrorDuration--;
      if(tankErrorDuration == 0)
      {
        lapCount++;
        strip.setPixelColor(5, strip.Color(0,0,0));
        strip.show();
        sendSerial("no_liquid_error_reset");
      }
    }
  }
  //sendSerial(outputString);
  //outputString = "";
  
}
//=========================================================================  
uint8_t checkString(String toSend, uint8_t error)
{
    if(toSend == "gateway_error"){
        gwError = 1;
    }
    if(toSend == "gateway_error_reset"){
        gwError = 0;
    }
    if(toSend == "gateway_power_error"){
        gwPowerError = 1;
    }
    if(toSend == "gateway_power_error_reset"){
        gwPowerError = 0;
    }
    if(toSend == "belt_plc_error"){
        plcError = 1;
    }
    if(toSend == "belt_plc_error_reset"){
        plcError = 0;
        beltErrorReset = 1;
        //Serial.println("LOSZAR");
    }
    if(toSend == "restart_system"){
        restart_mega();
    }
//    if(toSend == "go_to_sleep"){
//        delay(1000);
//        go_to_sleep();
//    }
    return error;
}
void sendSerial(String data)
{
  //Serial.flush();
  if(data.length() > 0)
  {
    Serial.print(data);
    delay(300);
  }
  data = "";
}
void restart_mega()
{
  free(inputCh);
  asm volatile ("  jmp 0");
}
void beltError(uint8_t ledState)
{
  uint8_t leds;
  if(ledState == 1)
  {
    leds = ledNumOn;
  } 
  else
  {
    leds = LEDS_SIDE_NUM;
  }
//  Serial.print("LEDS: ");
//  Serial.println(leds);
//  Serial.print("NUMON: ");
//  Serial.println(ledNumOn);
  if(plcError)
    {
      
      toggle = toggle? 0 : 1;
      for(uint8_t k = 0;k <= leds; k++)
      {
        strip2.setPixelColor(k, toggle ? ERROR_COLOR : OFF_COLOR);
      }
      for(uint8_t k = leds+1;k <= LEDS_SIDE_NUM; k++)
      {
        strip2.setPixelColor(k, OFF_COLOR);
      }
      strip2.show();
    }
  if(beltErrorReset)
    {
      
      for(uint8_t k = 0;k <= leds; k++)
      {
        
        if(ledState == 1)
        {
          strip2.setPixelColor(k, SIDE_COLOR);
        }
        else
        {
          if(ledsAreOn)
          {
            strip2.setPixelColor(k, SIDE_COLOR);
          }
          else
          {
            strip2.setPixelColor(k, OFF_COLOR);
          }
        }
      }
      for(uint8_t k = leds+1;k <= LEDS_SIDE_NUM; k++)
      {
        strip2.setPixelColor(k, OFF_COLOR);
      }
      strip2.show();
      beltErrorReset = 0;
    }
  
}
void turnoffWipe_side(uint32_t c, uint8_t wait)
{
  for(uint8_t i = LEDS_SIDE_NUM; i > 0; i--){
    strip2.setPixelColor(i,c);
    strip2.show();
    delay(wait);
  }
  strip2.setPixelColor(0,c);
  strip2.show();
}
void colorWipe(uint32_t c, uint8_t wait) 
{
  for (uint8_t i = 0; i < LEDS_TANK_NUM; i++) {
    strip.setPixelColor(i, c);
    delay(wait);
  }
     strip.show();
}
