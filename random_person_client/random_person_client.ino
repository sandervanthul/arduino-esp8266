#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <Bounce2.h>
#include "secrets.h"

#define SCREEN_WIDTH 128 // in pixels
#define SCREEN_HEIGHT 64 // in pixels

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define BUTTON_PIN D5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Bounce button = Bounce(); // debouncer

BearSSL::WiFiClientSecure wifiClient;
HTTPClient httpClient;
const char* serverurl = "https://randomuser.me/api";
const char* fingerprint = "61:18:A5:DD:47:C4:5B:79:51:A3:4C:94:91:D8:CD:67:E5:85:A8:1E";
JsonDocument doc;

struct Person {
  String firstName;
  String lastName;
  String country;
  int age;
  bool valid; // for guard pattern
};

void setup() {
  initSerial();
  initDisplay();
  connectToWifi();
  initButton();
}

void loop() {
  button.update();

  if (button.fell()) { // falling edge: button press counts once independent of duration pressed
    Serial.println("Button pressed!");
    Person person = getPersonData();
    if (person.valid){
      displayPersonData(person);
    }
    else {
      displayPersonRetrievalError();
    }
  }
}

void initSerial() {
  unsigned long baud = 9600;
  Serial.begin(baud);
  while (!Serial) {;}
  Serial.println("Serial initialized!");
  delay(500);
}

void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Display initialization failed!");
    for (;;); // stop execution if initialization failed
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Display Initialized!");
  display.display();
  Serial.println("Display initialized!");
  delay(500);
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    delay(500);
  }
  Serial.println("connected to WiFi!");

  wifiClient.setFingerprint(fingerprint); // enable https
  delay(500);
}

void initButton() {
  button.attach(BUTTON_PIN, INPUT_PULLUP);
  button.interval(25); // debounce time in ms
}

Person getPersonData() {
  Person person;
  person.valid = false;

  httpClient.begin(wifiClient, serverurl);
  int httpCode = httpClient.GET();

  if (httpCode <= 0) {
    Serial.printf("HTTP GET failed, error: %s\n", httpClient.errorToString(httpCode).c_str());
    httpClient.end(); // close connection
    return person;
  }

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Unexpected HTTP code: %d\n", httpCode);
    httpClient.end(); // close connection
    return person;
  }

  /********************************************* 
   At this point the http code should be 200 OK 
  *********************************************/ 

  String payload = httpClient.getString();
  Serial.println(payload);
  httpClient.end(); // close connection

  doc = DynamicJsonDocument(2048);

  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      return person;
  }

  // Extract fields with safe defaults (randomuser.me)
  JsonObject personJson = doc["results"][0].as<JsonObject>();
  person.firstName = personJson["name"]["first"] | "unknown";
  person.lastName  = personJson["name"]["last"]  | "unknown";
  person.country   = personJson["location"]["country"] | "unknown";
  person.age       = personJson["dob"]["age"] | 0;

  Serial.printf("Name: %s %s\n", person.firstName, person.lastName);
  Serial.printf("Country: %s\n", person.country);
  Serial.printf("Age: %d\n", person.age);

  person.valid = true;
  return person;
}

void displayPersonData(const Person& person) {
  // reset display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // provide data to display
  display.setCursor(0, 0);
  display.print("Name: ");
  display.print(person.firstName);
  display.print(" ");
  display.println(person.lastName);

  display.setCursor(0, 16);
  display.print("Country: ");
  display.println(person.country);

  display.setCursor(0, 32);
  display.print("Age: ");
  display.println(person.age);

  // show everything on display
  display.display(); 
}

void displayPersonRetrievalError() {
  // reset display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // provide data to display
  display.setCursor(0, 0);
  display.print("Something went wrong when retrieving person, nothing to display");

  // show everything on display
  display.display(); 
}




