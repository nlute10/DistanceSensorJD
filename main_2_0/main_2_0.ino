#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "Adafruit_VL53L0X.h"
#include <Wire.h>

Adafruit_VL53L0X lox = Adafruit_VL53L0X();


// Set inputs and outputs
#define TFT_CS   14 //Pin connected to chip select on the tft driver
#define TFT_RST  15//Pin connected to reset on the tft driver
#define TFT_RS   16 //Pin connect to register select on the tft driver
//#define TFT_SDA  17 //MOSI
//#define TFT_SCL  18  //sdk
#define TFT_SDA  11 //MOSI (default hardware SPI)
#define TFT_SCL  12 //SCK  (default hardware SPI)
#define SDA_PIN 8
#define SCL_PIN 9
#define BUTTON_PIN 4
#define XSHUT_PIN 7 //sensor reset pin



const int W = 128, H = 128;
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_RS, TFT_SDA, TFT_SCL, TFT_RST);// software SPI
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_RS, TFT_RST); // hardware SPI
GFXcanvas16 canvas(W,H);  // screen buffer

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b){
  return ((r& 0xF8) << 8) | ((g& 0xFC) << 3) | (b >> 3);
}

//Set color values
const uint16_t BLACK   = rgb(0,0,0);
const uint16_t WHITE   = rgb(255,255,255);
const uint16_t GREY    = rgb(150,150,150);
const uint16_t GREEN   = rgb(0, 220, 80);
const uint16_t ORANGE = rgb(255,160,0);
const uint16_t BLUE    = rgb(80,140,255);
const uint16_t RED     = rgb(255, 40, 40);
int distanceMM = 0;   // latest reading used in loop() and drawScreen()
bool outOfRange = true;
bool frozen = false;
bool lastButton = HIGH;
unsigned long pressStartButtonHold = 0;   
//float measureDistance(){

//Prints text in the center of the screen
void centerText(const char* s, int y, uint8_t size, uint16_t color) {
  int16_t x1, y1;
  uint16_t w, h;
  canvas.setTextSize(size);
  canvas.getTextBounds(s, 0, y, &x1, &y1, &w, &h);
  canvas.setCursor((W-w) / 2,y );
  canvas.setTextColor(color);
  canvas.print (s);
}




void drawScreen() {
  const int y_offset = 40;



  canvas.setTextSize(3);
  canvas.setTextWrap(false);

  // Current Distance Bar
  canvas.fillScreen(BLACK);
  if (frozen) {
    centerText("HOLD",10,2, RED);
  }
  //Sense the distance
  if (outOfRange == true) {
    centerText("--",y_offset, 3, WHITE);
    centerText("cm", 32+y_offset, 2, WHITE);
  } else {
  int distance = distanceMM ; //previosuly had lox.readRange() which cases the sensor to just take out another reading instead well save it too distanceMM to use the same number
  if (distance < 0) distance = 0;          // never show a negative number
  
  char buf[8];
  snprintf(buf, sizeof(buf), "%.1f", distance / 10.0);    

  centerText(buf,y_offset, 3, WHITE);
  centerText("cm", 32+y_offset, 2, WHITE);
  }
  tft.drawRGBBitmap(0,0,canvas.getBuffer(),W,H);
}



void setup(){
  Serial.begin(115200);

  tft.initR(INITR_144GREENTAB);
  tft.setSPISpeed(8000000);   
  Serial.println("Starting TFT");
  tft.setRotation(0);
  tft.fillScreen(BLACK); //Prevents grey screen after startup to see if its working properly
  // wait until serial port opens for native USB devices
  // while (! Serial) {
  //   delay(1);
  // } Got rid of this becaseu if the serial monitor doesnt open up it will just display grey static

  // Serial.println("Adafruit VL53L0X test.");
  // if (!lox.begin()) {
  //   Serial.println(F("Failed to boot VL53L0X"));
  //   while(1);
  // }
  // wait up to 2 seconds for Serial, then keep going
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 2000) {
    delay(10);
  }

  Serial.println("Adafruit VL53L0X test.");

    pinMode(XSHUT_PIN, OUTPUT);
  digitalWrite(XSHUT_PIN, LOW);    // sensor OFF
  delay(10);
  digitalWrite(XSHUT_PIN, HIGH);   // sensor ON
  delay(10);                       // give it time to wake up


  Wire.begin(SDA_PIN, SCL_PIN);

if (!lox.begin(0x29, false, &Wire,
    Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE)) {
    Serial.println(F("Failed to boot VL53L0X"));
    tft.setCursor(5, 50);
    tft.setTextColor(ORANGE);
    tft.setTextSize(1);
    tft.print("VL53L0X not found");   // show the error ON the screen
    while (1) delay(100);
  }

  // power
  Serial.println(F("VL53L0X API Continuous Ranging example\n\n"));

  // start continuous ranging
  lox.startRangeContinuous();
  
pinMode(BUTTON_PIN, INPUT_PULLUP);
lastButton = digitalRead(BUTTON_PIN);
  
  
  
}

unsigned long previousMillis = 0;
const unsigned long interval = 100; //previously 50. But it casues the screen to flicker a lot so
//change it too 200 so it redraws 5 times a sec

// millis is milliseconds since boot
void loop(){
  
  if (frozen == false && lox.isRangeComplete()) {

  uint16_t raw = lox.readRange();

  if (lox.readRangeStatus() == 0) {

    // First valid measurement
    if (distanceMM == 0 || outOfRange) {
      distanceMM = raw;
    } else {
      // Smooth distance measurements
      distanceMM = (distanceMM * 3 + raw + 2) / 4;
    }

    outOfRange = false;

  } else {
    outOfRange = true;
  }
}
  //calculate distance using speed of sound (m/s) at sea level



  


  

  bool button = digitalRead(BUTTON_PIN);
  if (lastButton == HIGH && button == LOW ) {
    frozen = !frozen;
    pressStartButtonHold = millis();
    delay(50);
  }
  lastButton = button;
  
  if(button == LOW && millis() - pressStartButtonHold > 2000) {
    tft.fillScreen(BLACK);
    tft.setCursor(25, 60);
    tft.setTextColor(GREEN);
    tft.setTextSize(1);
    tft.print("Restarting...");
    delay(1000);
    ESP.restart();
  }
    unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval){
  // Print data for serial monitor
    previousMillis = currentMillis;
    
    drawScreen();
  }
}