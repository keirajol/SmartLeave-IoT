#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// SETTINGS
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* API_KEY = "YOUR_OPENWEATHER_API_KEY";

// Amsterdam, Change these coordinates to the location you want to use.
const float LATITUDE = 52.3676;
const float LONGITUDE = 4.9041;

#define LED_PIN 4
#define BUTTON_PIN 13
#define LED_COUNT 30
#define LED_BRIGHTNESS 80

// Ten minutes 
const unsigned long UPDATE_INTERVAL = 10UL * 60UL * 1000UL;

WiFiClientSecure client;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

bool reminderActive = false;
bool rainAcknowledged = false;
unsigned long lastWeatherCheck = 0;

void setBlueLight(bool on) {
  if (on) {
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 255));
    }
  } else {
    strip.clear();
  }
  strip.show();
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected");
}

// source: chatGPT
// Checks the next 6 hours 
// Returns true when OpenWeather predicts Rain, Drizzle or Thunderstorm.
bool rainExpectedSoon() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  client.setInsecure(); 

  String url = "/data/2.5/forecast?lat=" + String(LATITUDE, 4) +
               "&lon=" + String(LONGITUDE, 4) +
               "&cnt=2&appid=" + String(API_KEY) + "&units=metric";

  Serial.println("Checking OpenWeather forecast...");

  if (!client.connect("api.openweathermap.org", 443)) {
    Serial.println("Connection to OpenWeather failed");
    return false;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.openweathermap.org\r\n" +
               "User-Agent: SmartLeave/1.0\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.connected() && !client.available()) {
    if (millis() - timeout > 5000) {
      Serial.println("OpenWeather request timed out");
      client.stop();
      return false;
    }
    delay(10);
  }

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, client);
  client.stop();

  if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
    return false;
  }

  if (doc["cod"].as<String>() != "200") {
    Serial.println("OpenWeather returned an error");
    return false;
  }

  bool rainSoon = false;
  JsonArray forecasts = doc["list"].as<JsonArray>();

  for (JsonObject forecast : forecasts) {
    const char* condition = forecast["weather"][0]["main"] | "";
    Serial.print("Forecast condition: ");
    Serial.println(condition);

    if (strcmp(condition, "Rain") == 0 ||
        strcmp(condition, "Drizzle") == 0 ||
        strcmp(condition, "Thunderstorm") == 0) {
      rainSoon = true;
    }
  }

  return rainSoon;
}

void updateReminder() {
  bool rainSoon = rainExpectedSoon();

  if (rainSoon) {
    Serial.println("Rain expected in the next 6 hours.");

    if (!rainAcknowledged) {
      setBlueLight(true);
      reminderActive = true;
      Serial.println("Smart Leave reminder ON: take an umbrella.");
    }
  } else {
    Serial.println("No rain expected in the next 6 hours.");
    setBlueLight(false);
    reminderActive = false;
    rainAcknowledged = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  setBlueLight(false);

  connectWiFi();

  // Check after startup.
  updateReminder();
  lastWeatherCheck = millis();
}

void loop() {
  // Physical confirmation: pressing the button turns the reminder off.
  if (reminderActive && digitalRead(BUTTON_PIN) == LOW) {
    delay(30); // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      setBlueLight(false);
      reminderActive = false;
      rainAcknowledged = true;
      Serial.println("Reminder acknowledged. Blue light OFF.");

      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  }

  if (millis() - lastWeatherCheck >= UPDATE_INTERVAL) {
    lastWeatherCheck = millis();
    updateReminder();
  }
}
