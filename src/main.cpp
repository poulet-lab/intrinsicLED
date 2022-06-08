#include <Arduino.h>
#include <Encoder.h>
#include <FastLED.h>
#include <Bounce2.h>

#define NUMLED  1//24
#define PINLED  17
#define PINBTN  3//15
#define PINENCA 4//26
#define PINENCB 7//14

CRGB leds[NUMLED];
int16_t brightness[] = {0, 255, 255};
uint8_t hue = 96;

Encoder enc(PINENCA, PINENCB);
int8_t  encDelta  = 0;

uint8_t mode = 0;

Bounce2::Button button = Bounce2::Button();

bool changes = true;
bool hueChange = false;

void setup() {
  Serial.begin(9600);

  button.attach(PINBTN, INPUT_PULLUP);
  button.interval(20); 
  button.setPressedState(LOW);

  FastLED.addLeds<WS2811,PINLED>(leds, NUMLED);
  FastLED.showColor(CRGB::Black);

  enc.write(0);
}

void loop() {

  // read the state of the push-button
  button.update();
  if (button.released()) {
    if (hueChange) {
      hueChange = false;
      return;
    }
    else {
      mode = (mode + 1) % 3;
      changes = true;
    }
  }
    
  // read encoder delta
  int8_t newEncPos = enc.read();
  if (newEncPos >=4 || newEncPos <= -4) {
    encDelta += ceil(newEncPos / 4.0);
    enc.write(0);
  }

  // change brightness / hue
  if (encDelta != 0) {
    if (button.isPressed()) {
      hueChange = true;
      if (mode == 1) {
        uint8_t newHue = hue - encDelta * 4;
        newHue = constrain(newHue, 96, 160);
        if (newHue != hue) {
          hue = newHue;
          changes = true;
        }
      }
    }
    else if (!button.isPressed() && mode > 0) {
      int16_t newBrightness = brightness[mode] + encDelta * 4;
      newBrightness = constrain(newBrightness, 0, 255);
      if (newBrightness != brightness[mode]) {
        brightness[mode] = newBrightness;
        changes = true;
      }
    }
  }
  encDelta = 0;

  // apply changes
  if (changes) {
    Serial.printf("mode = %d; brightness = %3d; hue = %3d\n",mode,brightness[mode],hue);
    switch (mode) {
      case 0:
        FastLED.showColor(CRGB::Black);
        break;
      case 1:
        FastLED.showColor(CHSV(hue, 255, brightness[mode]));
        break;
      case 2:
        FastLED.showColor(CHSV(0, 255, brightness[mode]));
        break;
    }
    changes = false;
  }
}
