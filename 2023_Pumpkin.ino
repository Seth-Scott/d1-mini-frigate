/*
using a D1 mini (ESP8266) board, animate LEDs (WS2812) triggered by frigate NVR person detection
basic script for animating halloween pumpkin LEDs when someone walks by the house
requires a frigate NVR setup connected to home assistant through mqtt server w/ HA API

resources:
https://cdn-shop.adafruit.com/datasheets/WS2812.pdf
https://frigate.video/
https://www.home-assistant.io/
https://developers.home-assistant.io/docs/api/rest/
https://www.wemos.cc/en/latest/d1/d1_mini.html
https://mosquitto.org/

my HA/frigate setup:
https://github.com/Seth-Scott/jetson_nano_frigateNVR

this could be achieved easier via esphome but I wanted more granular control through plain arduino code to expand later, if you're curious:
https://esphome.io/

don't forget ch340 driver to allow you to use the d1 mini with arduino IDE:
https://www.wemos.cc/en/latest/ch340_driver.html
*/

#include "credentials.h"
/*
IMPORTANT! need to create a credentials.h file with below (obfuscates passwords for uploading to github):
#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "wifi_password"
#define HA_IP "your_HA_instance_IP"
#define HA_TOKEN "your_HA_token"
*/

// wifi libraries
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
// LED libraries
#include <FastLED.h>
// GET request libraries
#include <ESP8266HTTPClient.h>
// JSON library for comparing GET request payloads
#include <ArduinoJson.h>

// number of LEDs connected
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

// i'm using individual WS2812 LED chips, could also use a LED ribbon
const int LED_PIN = D4;
// TODO remove, for debug
const int buzzerPin = D3;

// wifi setup
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// home assistant setup
const char* homeAssistantServer = HA_IP; // IP address of home assistant
const int homeAssistantPort = 8123; // home assistant port, typically 8123
const char* accessToken = HA_TOKEN; //home assistant access token
const char* homeAssistantAPIEndpoint = "/api/states/binary_sensor.front_door_person_occupancy?token=";

// Define a variable to store the previous "last_changed" value
String previousState = "";

// TODO figure out what this does
ESP8266WiFiMulti WiFiMulti;

void setup() {
  Serial.begin(9600);

  // instantiate LEDs 
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  pinMode(buzzerPin, OUTPUT);

  // instantiate wifi connection
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  // for serial debug
  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  // while the wifi isn't connected, wait 0.5 seconds before trying again
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // blink the LED green a few times to indicate that the D1 mini has successfully connected to wifi
  for (int i = 0; i <= 3; i++) {
    leds[0] = CRGB::Green; FastLED.show(); 
    delay(300);
	  leds[0] = CRGB::Black; FastLED.show(); 
    delay(300);
  }
}

// dummy function for LED animation, function is expecting quantity of times to run
void runLEDAnimation(int numIterations) {
  for (int j = 0; j < numIterations; j++) {
    leds[0] = CRGB::Orange;
    FastLED.show();
    delay(300);
    leds[0] = CRGB::Purple;
    FastLED.show();
    delay(300);
  }
}

// TODO delete when ready, this is for testing
void runBuzzer(int numIterations) {
  for (int j = 0; j < numIterations; j++) {
    digitalWrite(buzzerPin, HIGH);
    delay(300);
    digitalWrite(buzzerPin, LOW);
    delay(300);
  }
}

void loop() {
  // wait a moment before making the HTTP request
  delay(1000); // Wait for 1 second

  // create WiFiClient object
  WiFiClient client;

  // construct URL for the GET request
  // adjust as needed, my api endpoint looks like this "/api/states/binary_sensor.front_door_person_occupancy?token="
  String url = "http://" + String(homeAssistantServer) + ":" + String(homeAssistantPort) + homeAssistantAPIEndpoint + String(accessToken);

  // create an HTTPClient object and pass the WiFiClient and URL
  HTTPClient http;
  http.begin(client, url); // Use the updated begin method

  // set authorization header with the access token
  String authorizationHeader = "Bearer " + String(accessToken);
  http.addHeader("Authorization", authorizationHeader);

  // send GET request
  int httpResponseCode = http.GET();

  // check for a successful response
  if (httpResponseCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Response: " + payload);

    // parse JSON response
    StaticJsonDocument<512> jsonDocument; // Adjust the size as needed
    DeserializationError error = deserializeJson(jsonDocument, payload);

    // check for parsing errors
    if (error) {
      Serial.print("Error parsing JSON: ");
      Serial.println(error.c_str());
    } else {
      // extract the "state" value
      const char* state = jsonDocument["state"];

      // check if the state changed from "off" to "on"
      if (String(state) == "on" && previousState == "off") {
        // Execute the function when the state changes
        runBuzzer(4);
      }
      
      // store current state for the next iteration
      previousState = String(state);
    }
  } else {
    Serial.print("Error - HTTP Response Code: ");
    Serial.println(httpResponseCode);
  }

  // end request
  http.end();

  // wait before making the next request (adjust the delay as needed)
  delay(1000);
}
