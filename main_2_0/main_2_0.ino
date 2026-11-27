#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "Adafruit_VL53L0X.h"
#include <Wire.h>

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// Set inputs and outputs
#define TFT_CS   14 // Pin connected to chip select on the tft driver
#define TFT_RST  15 // Pin connected to reset on the tft driver
#define TFT_RS   16 // Pin connected to register select on the tft driver
#define TFT_SDA  11 // MOSI (default hardware SPI)
#define TFT_SCL  12 // SCK  (default hardware SPI)
#define SDA_PIN 8
#define SCL_PIN 9
#define BUTTON_PIN 4
#define XSHUT_PIN 7 // sensor reset pin

const int W = 128, H = 128;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_RS, TFT_RST); // hardware SPI
GFXcanvas16 canvas(W, H);  // screen buffer

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Set color values
const uint16_t BLACK  = rgb(0, 0, 0);
const uint16_t WHITE  = rgb(255, 255, 255);
const uint16_t GREY   = rgb(150, 150, 150);
const uint16_t GREEN  = rgb(0, 220, 80);
const uint16_t ORANGE = rgb(255, 160, 0);
const uint16_t BLUE   = rgb(80, 140, 255);
const uint16_t RED    = rgb(255, 40, 40);

// Averaging
int distanceMM = 0;               // averaged value used by drawScreen()
long sumMM = 0;                   // running sum for the current batch
int sampleCount = 0;              // samples collected in the current batch
const int NUM_SAMPLES = 3;
const int MAX_RANGE_MM = 2000;    // long range mode tops out around 2 m; 8190/8191/65535 = no target

bool outOfRange = false;
bool frozen = false;
bool lastButton = HIGH;
unsigned long pressStartButtonHold = 0;

unsigned long previousMillis = 0;
const unsigned long interval = 200; // redraw 5 times a second

// Prints text in the center of the screen
void centerText(const char* s, int y, uint8_t size, uint16_t color) {
  int16_t x1, y1;
  uint16_t w, h;
  canvas.setTextSize(size);
  canvas.getTextBounds(s, 0, y, &x1, &y1, &w, &h);
  canvas.setCursor((W - w) / 2, y);
  canvas.setTextColor(color);
  canvas.print(s);
}

void drawScreen() {
  const int y_offset = 40;

  canvas.setTextSize(3);
  canvas.setTextWrap(false);
  canvas.fillScreen(BLACK);

  if (frozen) {
    centerText("HOLD", 10, 2, RED);
  }

  if (outOfRange) {
    centerText("--", y_offset, 3, WHITE);
    centerText("cm", 32 + y_offset, 2, WHITE);
  } else {
    int distance = distanceMM;
    if (distance < 0) distance = 0;   // never show a negative number

    char buf[8];
    snprintf(buf, sizeof(buf), "%.1f", distance / 10.0);

    centerText(buf, y_offset, 3, WHITE);
    centerText("cm", 32 + y_offset, 2, WHITE);
  }
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), W, H);
}

void setup() {
  Serial.begin(115200);

  tft.initR(INITR_144GREENTAB);
  tft.setSPISpeed(8000000);
  Serial.println("Starting TFT");
  tft.setRotation(180);
  tft.fillScreen(BLACK); // Prevents grey screen after startup

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
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    tft.setCursor(5, 50);
    tft.setTextColor(ORANGE);
    tft.setTextSize(1);
    tft.print("VL53L0X not found");   // show the error ON the screen
    while (1) delay(100);
  }

  // Long range mode (must be set before starting continuous ranging)
  lox.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE);
  // Longer timing budget = better range and accuracy (50 ms per reading)
  if (!lox.setMeasurementTimingBudgetMicroSeconds(50000)) {
    Serial.println(F("Failed to set timing budget"));
  }

  Serial.println(F("VL53L0X long range continuous ranging\n"));
  lox.startRangeContinuous();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  lastButton = digitalRead(BUTTON_PIN);
}

void loop() {
  // ---- Sensor reading ----
  if (!frozen && lox.isRangeComplete()) {
    uint16_t raw = lox.readRange();

    if (raw > MAX_RANGE_MM) {
      outOfRange = true;
      sumMM = 0;
      sampleCount = 0;
    } else {
      outOfRange = false;
      sumMM += raw;
      sampleCount++;

      if (sampleCount >= NUM_SAMPLES) {
        distanceMM = sumMM / NUM_SAMPLES;
        sumMM = 0;
        sampleCount = 0;
      }
    }
  }

  // ---- Button (always checked) ----
  bool button = digitalRead(BUTTON_PIN);
  if (lastButton == HIGH && button == LOW) {
    frozen = !frozen;
    pressStartButtonHold = millis();
    delay(50);
  }
  lastButton = button;

  // Hold for 2 seconds to restart
  if (button == LOW && millis() - pressStartButtonHold > 2000) {
    tft.fillScreen(BLACK);
    tft.setCursor(25, 60);
    tft.setTextColor(GREEN);
    tft.setTextSize(1);
    tft.print("Restarting...");
    delay(1000);
    ESP.restart();
  }

  // ---- Display ----
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    drawScreen();
  }
}
