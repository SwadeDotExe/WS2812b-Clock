/* SK6812 Clock Driver

   Drives the 3D printed clock in my dorm room. Runs off
   the ESP8266 and is controlled through MQTT with time
   being pulled from a RTC.

   Written by Swade Cirata

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
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>

// WiFi Credentials
const char ssid[] = "SSID";                  // your network SSID (name)
const char password[] = "PASS";    // your network password (use for WPA, or use as key for WEP)
const char mqtt_server[] = "192.168.1.208";   // MQTT Server address

/* Define LED Parameters */
#define NUM_LEDS 214
#define DATA_PIN 14 // D5

/* LED Data */
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];

/* Brightness */
uint8_t brightness = 128;

/* Number Pixel Mappings */
const int numberMap[10][50] = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34}, // Digit 0
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},                                                          // Digit 1
  {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34}, // Digit 2
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 20, 21, 22, 23, 24, 30, 31, 32, 33, 34}, // Digit 3
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},                  // Digit 4
  {0, 1, 2, 3, 4, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 30, 31, 32, 33, 34}, // Digit 5
  {0, 1, 2, 3, 4, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34}, // Digit 6
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},                                      // Digit 7
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31}, // Digit 8
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24}
};                      // Digit 9

/* Digit Offset Mappings (start address of each digit) */
uint8_t digitMap[] = {0, 35, 70, 105, 140, 175, 210};

/* Time Variables */
uint8_t hour2;    // Second Hour Digit
uint8_t hour1;    //  First Hour Digit
uint8_t minute2;  // Second Minute Digit
uint8_t minute1;  //  First Minute Digit
uint8_t second2;  // Second Second Digit
uint8_t second1;  //  First Second Digit

/* UTC Time Offset */
const long utcOffsetInSeconds = 3600;

/* Previous Time Tracker */
uint8_t pastSecond;

// Setup MQTT
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup() {

  /* Begin Serial */
  Serial.begin(115200);

  /* Setup FastLED */
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setBrightness(brightness);
  FastLED.show();

  /* Pixel Test */
  pixelTest();

  // Setup WiFi
  setup_wifi();

  // Create MQTT Client
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Start NTP client
  timeClient.begin();

  /* Update time */
  timeClient.update();

  /* Get current second so loop can start */
  pastSecond = timeClient.getSeconds();

}

void loop() {

  /* Wait for next second */
  while (pastSecond == timeClient.getSeconds()) {
    timeClient.update();  // Update time
    delay(100);
  }

  /* Update blocker variable */
  pastSecond = timeClient.getSeconds();

  /* Set the LED clock time */
  setClockTime(timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());

}

/* Function to set the time on the clock */
void setClockTime(int hour, int minute, int second) {

  // /* Parse time for 6 digits of clock */
  // int secondRight =  second % 10;        // Segment 0
  // int secondLeft  = (second / 10U) % 10; // Segment 1
  // int minuteRight =  second % 10;        // Segment 2
  // int minuteLeft  = (second / 10U) % 10; // Segment 3
  // int hourRight   =  second % 10;        // Segment 4
  // int hourLeft    = (second / 10U) % 10; // Segment 5

  /* Set each segment to time */
  setClockSegment(0, second % 10);
  setClockSegment(1, (second / 10U) % 10);
  setClockSegment(2, minute % 10);
  setClockSegment(3, (minute / 10U) % 10);
//  setClockSegment(4, hour % 10);
  // setClockSegment(5, (hour / 10U) % 10);
}

/* Function to control each clock digit */
void setClockSegment(int segment, int time) {

  /* Variable to keep track of active pixel array */
  int arrayTracker = 0;

  /* Variable to keep track of pixel offset */
  int arrayOffset = 0;

  /* Set Seconds (1st Digit) */
  for (int a = digitMap[segment]; a < digitMap[segment + 1]; a++) { // Pixel Loop

    /* Checks if pixel should be on for given digit */
    if (numberMap[time][arrayTracker] == arrayOffset) { // Needs to be on
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
void controlPixel(int pixelNumber, CRGB color) {
  if (color == (CRGB) CRGB::White) {             // If color is white
    leds[pixelNumber] = CRGBW(0, 0, 0, 255);
    FastLED.show();
  }
  else {                              // If color is not white
    leds[pixelNumber] = color;
    FastLED.show();
  }
}
/* Sets all LEDs to Passed Color (for startup) */
void colorFill(CRGB c, int speed) {
  for (int i = 0; i < NUM_LEDS; i++) {  // Loop through all leds
    if (c == (CRGB) CRGB::White) {             // If color is white
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
  // colorFill(CRGB::Red, 1);
  // delay(100);
  // colorFill(CRGB::Green, 1);
  // delay(100);
  // colorFill(CRGB::Blue, 1);
  // delay(100);
  // colorFill(CRGB::White, 1);
  // delay(100);
  // colorFill(CRGB::Black, 1);
  delay(100);
  setClockTime(00, 00, 00);
  delay(100);
  setClockTime(11, 11, 11);
  delay(100);
  setClockTime(22, 22, 22);
  delay(100);
  setClockTime(33, 33, 33);
  delay(100);
  setClockTime(44, 44, 44);
  delay(100);
  setClockTime(55, 55, 55);
  delay(100);
  setClockTime(66, 66, 66);
  delay(100);
  setClockTime(77, 77, 77);
  delay(100);
  setClockTime(88, 88, 88);
  delay(100);
  setClockTime(99, 99, 99);
  delay(100);
  colorFill(CRGB::Black, 1);
}

/* Debugging only */
void printTime() {
  timeClient.update();
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
}

/* Setup Wifi */
void setup_wifi() {

  delay(10);
//  colorFill(CRGB::Blue, 10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
//    colorFill(CRGB::Black, 10);
    delay(250);
    Serial.print(".");
//    colorFill(CRGB::Blue, 10);
    delay(250);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

//  colorFill(CRGB::Green, 10);
//  delay(100);
//  colorFill(CRGB::Black, 10);
//  delay(100);
//  colorFill(CRGB::Green, 10);
//  delay(100);
//  colorFill(CRGB::Black, 10);
//  delay(100);
//  colorFill(CRGB::Green, 10);
//  delay(100);
//  colorFill(CRGB::Black, 10);
//  delay(100);
}

// Get MQTT Message (Interrupt)
void callback(char topic[], unsigned char payload[], unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  boolean breakOut = false;
  char buffer[length];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    buffer[i] = (char)payload[i];
  }
  Serial.println();

  // Convert buffer to String
  String message = String(buffer);

//  // Brightness Handler
//  if (strcmp(topic, "METARMap/Brightness") == 0) {
//
//    brightness = message.toInt();
//    Serial.print("Updating Brightness to ");
//    Serial.println(brightness);
//    oldBrightness = brightness;
//    FastLED.setBrightness(brightness);     // Set brightness of LEDs to variable
//    FastLED.show();
//    breakOut = true;
//    break;
//  }
//
//  // Right Couch Handler
//  if (strcmp(topic, "METARMap/State") == 0 && !breakOut) {
//    Serial.print("Updating state to ");
//    Serial.println(message);
//    if (message == "true") {
//      brightness = oldBrightness;
//      FastLED.setBrightness(brightness);     // Set brightness of LEDs to variable
//      FastLED.show();
//    } else {
//      oldBrightness = brightness;
//      brightness = 0;
//      FastLED.setBrightness(brightness);     // Set brightness of LEDs to variable
//      FastLED.show();
//    }
//  }
}

void MQTTReconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("METARMap/State");
      client.subscribe("METARMap/Brightness");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
