/* FastLED RGBW Example Sketch
 *
 * Example sketch using FastLED for RGBW strips (SK6812). Includes
 * color wipes and rainbow pattern.
 *
 * Written by David Madison
 * http://partsnotincluded.com
*/

#include "FastLED.h"
#include "FastLED_RGBW.h"

#define NUM_LEDS 220
#define DATA_PIN 6 // CLK pin 

CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];

const uint8_t brightness = 128;

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setBrightness(brightness);
  FastLED.show();
}

void loop(){
  colorFill(CRGB::Red);
  colorFill(CRGB::Green);
  colorFill(CRGB::Blue);
  fillWhite();
  rainbowLoop();
}

void colorFill(CRGB c){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = c;
    FastLED.show();
    delay(50);
  }
  delay(500);
}

void fillWhite(){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGBW(0, 0, 0, 255);
    FastLED.show();
    delay(50);
  }
  delay(500);
}

void rainbow(){
  static uint8_t hue;

  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CHSV((i * 256 / NUM_LEDS) + hue, 255, 255);
  }
  FastLED.show();
  hue++;
}

void rainbowLoop(){
  long millisIn = millis();
  long loopTime = 5000; // 5 seconds

  while(millis() < millisIn + loopTime){
    rainbow();
    delay(5);
  }
}
