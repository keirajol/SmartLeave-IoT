#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

char ssid[] = "KPNDBD266";
char pass[] = "H3qvLJhmwsLLsxLj";
const char* API_KEY = "API_KEY_HERE"; // place your API key

#define LED_PIN 4
#define BUTTON_PIN 13
#define LED_COUNT 30
#define LED_BRIGHTNESS 80
#define UPDATE_INTERVAL 60000

WiFiClientSecure client;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

bool reminderActive = false;
unsigned long lastCheck = 0;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  strip.show();

  WiFi.begin(ssid, pass);  
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  client.setInsecure(); 
}

// Turns on green reminder light
void showGreenReminder() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
  }
  strip.show();
  reminderActive = true;
}

// Turns off the LED strip
void clearLEDs() {
  strip.clear();
  strip.show();
}

void getWasteData() {
  Serial.println("Waste data received (prototype)");
  showGreenReminder();
}

void loop() {
  if (!reminderActive && millis() - lastCheck > UPDATE_INTERVAL) {
    lastCheck = millis();
    getWasteData();
  }

  // Button confirms the task (1 druk = uit)
  if (reminderActive && digitalRead(BUTTON_PIN) == LOW) {
    clearLEDs();
    reminderActive = false;
    Serial.println("Trash confirmed");

    while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    delay(50);
  }
}
