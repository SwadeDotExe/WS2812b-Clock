/* SK6812 Clock Driver
 *
 * Drives the 3D printed clock in my dorm room. Runs off
 * the ESP8266 and is controlled through MQTT with time
 * being pulled from a RTC.
 *
 * Written by Swade White

                    ESP8266 NodeMCU
               +-----------------------+
               | O      | USB |      O |
               |        -------        |
               | 3.3V              Vin |
               | GND               GND |
               | GPIO1             RST |
               | GPIO3              EN |
               | GPIO15           3.3V |
               | GPIO13            GND |
               | GPIO12           SCLK |
               | GPIO14           MISO |
               | GND                CS |
               | 3.3V             MOSI |
               | GPIO2           GPIO9 |
               | GPIO0          GPIO10 |
               | GPIO4             RSV |
               | GPIO5             RSV |
               | GPIO16           ADC0 |
               |                       |
               | O                   O |
               +-----------------------+
*/

/* Includes*/
#include "FastLED.h"
#include "FastLED_RGBW.h"
#include <Wire.h>
#include "RTClib.h"

/* Define LED Parameters */
#define NUM_LEDS 220
#define DATA_PIN 6

/* LED Data */
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];

/* Brightness */
uint8_t brightness = 128;

/* Number Pixel Mappings */
numberMap = [[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,25,26,27,28,29,30,31],                // Digit 0
             [0,1,2,3,4,5,6,7,8,9],                                                                   // Digit 1
             [5,6,7,8,9,10,11,12,13,14,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34],                 // Digit 2
             [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,20,21,22,23,24,30,31,32,33,34],                      // Digit 3
             [0,1,2,3,4,5,6,7,8,9,15,16,17,18,19,20,21,22,23,24],                                     // Digit 4
             [0,1,2,3,4,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,30,31,32,33,34],                 // Digit 5
             [0,1,2,3,4,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34],  // Digit 6
             [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14],                                                    // Digit 7
             [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31], // Digit 8
             [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]]                      // Digit 9

/* Digit Offset Mappings (start address of each digit) */
uint8_t digitMap = [0, 35, 144, 179, XXX, XXX];

/* Time Variables */
uint8_t hour2;    // Second Hour Digit
uint8_t hour1;    //  First Hour Digit
uint8_t minute2;  // Second Minute Digit
uint8_t minute1;  //  First Minute Digit
uint8_t second2;  // Second Second Digit
uint8_t second1;  //  First Second Digit

/* Previous Time Tracker */
uint8_t pastSecond;

/* Create instance of RTC */
RTC_DS1307 rtc;

/* Create variable for RTC values */
DateTime timeNow;

void setup() {
  /* Begin Serial */
  Serial.begin(9600);

  /* Setup FastLED */
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setBrightness(brightness);
  FastLED.show();

  /* Start RTC */
  if (!rtc.begin()) {
    while (1);
  }

  /* Set RTC date to current time */
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  /* Pixel Test */
  pixelTest();

  /* Read time to get initial value */
  timeNow = rtc.now();

  /* Store seconds value */
  pastSecond = timeNow.second();

}

void loop() {

  /* Wait for next second */
  while(pastSecond == timeNow.second()) {
    timeNow = rtc.now();  // Update time
  }

  /* Set the clock time */
  setClockTime(now.hour(), now.minute(), now.second());

}

/* Function to set the time on the clock */
void setClockTime(Integer hour, Integer minute, Integer second) {

  /* Parse time for 6 digits of clock */
  Integer secondRight =  second % 10;        // Segment 0
  Integer secondLeft  = (second / 10U) % 10; // Segment 1
  Integer minuteRight =  second % 10;        // Segment 2
  Integer minuteLeft  = (second / 10U) % 10; // Segment 3
  Integer hourRight   =  second % 10;        // Segment 4
  Integer hourLeft    = (second / 10U) % 10; // Segment 5

  /* Set each segment to time */
  setClockSegment(0, secondRight);
  setClockSegment(1, secondLeft);
  setClockSegment(2, minuteRight);
  setClockSegment(3, minuteLeft);
  setClockSegment(4, hourRight);
  setClockSegment(5, hourLeft);
}

/* Function to control each clock digit */
void setClockSegment(Integer segment, Integer time) {

  /* Variable to keep track of active pixel array */
  Integer arrayTracker = 0;

  /* Variable to keep track of pixel offset */
  Integer arrayOffset = 0;

  /* Set Seconds (1st Digit) */
  for(Integer a = digitMap[segment]; a < digitMap[segment + 1]; a++) {  // Pixel Loop

    /* Checks if pixel should be on for given digit */
    if(numberMap[time][arrayTracker] == arrayOffset) { // Needs to be on
      // Modify pixel
      controlPixel(a, CRGB::White);
      // Increase arrayTracker since match was found
      arrayTracker++;
    }
    else {  // Needs to be off
      // Modify pixel
      controlPixel(a, CRGB::Black);
    }

    /* Increment Offset */
    arrayOffset++;
  }
}

/* Function to set parameters of individual pixel */
void controlPixel(Integer pixelNumber, CRGB color) {
    if(color == CRGB::White) {              // If color is white
      leds[pixelNumber] = CRGBW(0, 0, 0, 255);
      FastLED.show();
    }
    else {                              // If color is not white
      leds[pixelNumber] = c;
      FastLED.show();
    }

/* Sets all LEDs to Passed Color (for startup) */
void colorFill(CRGB c, Integer speed){
  for(int i = 0; i < NUM_LEDS; i++){    // Loop through all leds
    if(c == CRGB::White) {              // If color is white
      leds[i] = CRGBW(0, 0, 0, 255);
      FastLED.show();
    }
    else {                              // If color is not white
      leds[i] = c;
      FastLED.show();
    }
    delay(speed);                       // Delay pixel progression
  }
}

/* Loops through colors to test pixels (and look cool) */
void pixelTest() {
  colorSweep(CRGB::Red, 10);
  delay(100);
  colorSweep(CRGB::Green, 10);
  delay(100);
  colorSweep(CRGB::Blue, 10);
  delay(100);
  colorSweep(CRGB::White), 10;
  delay(100);
  colorSweep(CRGB::Black), 10;
  delay(100);
}

/* Debugging only */
void printTime() {
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");
}
