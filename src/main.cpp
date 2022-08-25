#include <Arduino.h>
#include <EEPROM.h>
#include <FastLED.h>
#include "EncoderTool.h"
using namespace EncoderTool;

#define NUMLED   24
#define PINLED   17
#define PINBTN    3
#define PINENCA   4
#define PINENCB   7

#define OFF       0
#define WHITE     1
#define GREEN     2
#define RED       3

#define PRESSED   0
#define RELEASED  1

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

CRGB leds[NUMLED];
int16_t brightness[] = {0, 255, 255, 255};
uint8_t hue = 96;
uint8_t chipset = 0;

PolledEncoder enc;

uint8_t mode       = OFF;
bool changes       = true;
bool hueChange     = false;
uint32_t timeStamp = 0;

void cbButton(int state) {
  static bool firstRun = true;
  
  if (firstRun) {
    firstRun = false;
    return;
  }
  if (state==RELEASED) {
    if (hueChange)
      hueChange = false;
    else {
      mode = (mode + 1) % 4;
      changes = true;
    }
  } else if (mode==OFF)
    timeStamp = millis();
}

void cbEncoder(int state, int delta) {
  if (enc.getButton()==RELEASED && mode > 0) {
    int16_t newBrightness = brightness[mode] + delta * 4;
    newBrightness = constrain(newBrightness, 0, 255);
    if (newBrightness != brightness[mode]) {
      brightness[mode] = newBrightness;
      changes = true;
    }
  } else if (enc.getButton()==PRESSED && mode == GREEN) {
    hueChange = true;
    uint8_t newHue = hue - delta * 4;
    newHue = constrain(newHue, 96, 160);
    if (newHue != hue) {
      hue = newHue;
      changes = true;
    }
  }
}

void setup() {
  Serial.begin(9600);

  chipset = EEPROM.read(0);
  if (chipset > 1) {
    chipset = 0;
    EEPROM.update(0, chipset);
  }
  switch (chipset) {
    case 0:
      FastLED.addLeds<WS2811,PINLED>(leds, NUMLED);
      break;
    case 1:
      FastLED.addLeds<NEOPIXEL,PINLED>(leds, NUMLED);
      break;
  }
  FastLED.showColor(CRGB::Black);

  enc.begin(PINENCA, PINENCB, PINBTN, CountMode::quarter, INPUT_PULLUP);
  enc.attachCallback(cbEncoder);
  enc.attachButtonCallback(cbButton);
}

void loop() {
  enc.tick();

  if (changes) {
    switch (mode) {
      case OFF:
        FastLED.showColor(CRGB::Black);
        break;
      case WHITE:
        FastLED.showColor(CRGB::White, brightness[mode]);
        break;
      case GREEN:
        FastLED.showColor(CHSV(hue, 255, brightness[mode]));
        break;
      case RED:
        FastLED.showColor(CRGB::Red, brightness[mode]);
    }
    changes = false;

  } else if (mode==OFF && enc.getButton()==PRESSED && millis()-timeStamp>10000) {
    chipset = (chipset + 1) % 2;
    EEPROM.update(0, chipset);
    FastLED.showColor(CRGB::White, 32);
    delay(50);
    FastLED.showColor(CRGB::Black);
    delay(50);
    FastLED.showColor(CRGB::White, 32);
    delay(50);
    FastLED.showColor(CRGB::Black);
    CPU_RESTART;
  }
}
