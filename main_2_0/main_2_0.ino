#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "Adafruit_VL53L0X.h"

Adafruit_VL53L0X lox = Adafruit_VL53L0X();


// Set inputs and outputs
#define TFT_CS   14 //Pin connected to chip select on the tft driver
#define TFT_RST  15//Pin connected to reset on the tft driver
#define TFT_RS   16 //Pin connect to register select on the tft driver
#define TFT_SDA  17 //MOSI
#define TFT_SCL  18  //sdk
#define SDA_PIN 8   //TOF sensor DATA pin
#define SCL_PIN 9   //TOF sensor Clock pin

//-------------------------------------------Display -----------------------------------------

//Define TFT Size
const int W = 128, H = 128;
 Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_RS, TFT_SDA, TFT_SCL, TFT_RST);// software SPI
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_RS, TFT_RST); // hardware SPI
GFXcanvas16 canvas(W,H);  // screen buffer

// idk what this does
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



//Prints text in the center of the screen 
//**TODO** needs to adjust for different sizes of text
void centerText(const char* s, int y, uint16_t color) {
  int w = strlen(s) * 6;
  canvas.setCursor((W-w) / 2,y );
  canvas.setTextColor(color);
  canvas.print (s);
}



//Draws the screen and converts the distance int to a char
void drawScreen() {
  const int y_offset = 0;

  canvas.fillScreen(BLACK);
  canvas.setTextSize(3);
  canvas.setTextWrap(false);

  // Current Distance Bar
  canvas.fillRect(0, 2 + y_offset, W, 120, GREEN);
  
  int distance = lox.readRange();
  char buf[6];
  snprintf(buf, sizeof(buf), "%u", distance);
  centerText(buf, 18 + y_offset, BLACK);
  centerText("cm", 40+y_offset, BLACK);

  tft.drawRGBBitmap(0,0,canvas.getBuffer(),W,H);
}


//First cycle program
void setup(){
  Serial.begin(115200);

  // wait until serial port opens for native USB devices
  while (! Serial) {
    delay(1);
  }

  Serial.println("Adafruit VL53L0X test.");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  // power
  Serial.println(F("VL53L0X API Continuous Ranging example\n\n"));

  // start continuous ranging, reports position every cycle
  lox.startRangeContinuous();
  
  tft.initR(INITR_144GREENTAB);
  Serial.println("Starting TFT");
  tft.setRotation(0);

  
  
  
}

unsigned long previousMillis = 0;
const unsigned long interval = 100; //100ms

// millis is milliseconds since boot

// Repeat this while the arduino is on
void loop(){
  
  

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval){
  

  //calculate distance using speed of sound (m/s) at sea level
  
 

  Serial.print("Reading a measurement...");
   if (lox.isRangeComplete()) {
    Serial.print("Distance in mm: ");
    Serial.println(lox.readRange());
  }


  // Print data for serial monitor
    Serial.print("Distance: " );
    
    Serial.println(" cm");

    previousMillis = currentMillis; //advance the time
    
    drawScreen();
  }
  

}



