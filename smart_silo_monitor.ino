/*

  Livestock Feed Level Monitoring System - Deep Sleep Version
  ESP32 + Ultrasonic Sensor (HC-SR04 / JSN-SR04T) + Telegram Bot

    - Wakes up every 2 hours to measure
    - Sends report and alerts via Telegram
    - Goes back to deep sleep to save power
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>


const char TELEGRAM_CERTIFICATE_ROOT[] PROGMEM = "-----BEGIN CERTIFICATE-----\nMIIE0zCCA7ugAwIBAgIQGrfZNOBGgoL7l7eWzH4wDDAKBggqhkjOPQQDAjBOMQsw\nCQYDVQQGEwJVUzETMBEGA1UEChMKQW1hem9uMRkwFwYDVQQDExBBbWF6b24gUm9v\ndCBDQSAxMB4XDTIwMDUyNjAwMDAwMFoXDTQwMDUyNjAwMDAwMFowTjELMAkGA1UE\nBhMCVVMxEzARBgNVBAoTCkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJvb3QgQ0Eg\nMTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABF5fmhZb7nO5QWJqZlHcR8k8l5K5\nZQpQ7ZQpQ7ZQpQ7ZQpQ7ZQpQ7ZQpQ7ZQpQ7ZQpQ7ZQpQ7ZQpQ7ZQpQ7ZQpQ7ZQpQw\nCgYIKoZIzj0EAwIDSQAwRgIhAK5Z5K5Z5K5Z5K5Z5K5Z5K5Z5K5Z5K5Z5K5Z5K5Z5\nAiEA5K5Z5K5Z5K5Z5K5Z5K5Z5K5Z5K5Z5K5Z5K5Z5K5Z5A==\n-----END CERTIFICATE-----\n";

//  SETTINGS - MUST BE CHANGED 
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* BOT_TOKEN     = "YOUR_TELEGRAM_BOT_TOKEN";
const char* CHAT_ID       = "YOUR_CHAT_ID";

// Silo dimensions in centimeters (measure once during installation)
const float SILO_HEIGHT_CM   = 200.0;  // Distance from sensor to bottom of empty silo
const float SENSOR_OFFSET_CM = 5.0;    // Height of sensor body above silo edge (set 0 if unknown)

// Alert thresholds (%)
const int LOW_LEVEL_THRESHOLD = 20;
const int CRITICAL_THRESHOLD  = 10;

//  PIN CONNECTIONS 
#define TRIG_PIN 5    // TRIG -> GPIO5 (Direct connection, 3.3V logic is accepted by most modules)
#define ECHO_PIN 18   // ECHO -> GPIO18 (Must use voltage divider if sensor is powered by 5V)

//  DEEP SLEEP SETTINGS 
const uint64_t SLEEP_TIME_US = 2 * 60 * 60 * 1000000ULL; // 2 hours in microseconds
const int NUM_SAMPLES = 5;                               // Average of 5 readings to reduce noise

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

//  Single ultrasonic distance measurement with Timeout 
float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000UL); // 30ms timeout (~5m max range)
  if (duration == 0) return -1;                     // Failed / out of range
  return duration * 0.0343f / 2.0f;                 // Speed of sound 343 m/s
}

//  Average multiple readings and ignore outliers 
float getAveragedDistance() {
  float sum = 0;
  int validReadings = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    float d = readDistanceCM();
    if (d > 0 && d < 500) {
      sum += d;
      validReadings++;
    }
    delay(60);
  }
  if (validReadings == 0) return -1;
  return sum / validReadings;
}

//  Convert measured distance to fill percentage 
int calculateFillPercent(float distanceCM) {
  float usableHeight = SILO_HEIGHT_CM - SENSOR_OFFSET_CM;
  float feedHeight = usableHeight - (distanceCM - SENSOR_OFFSET_CM);
  float percent = (feedHeight / usableHeight) * 100.0f;
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  return (int)percent;
}

void sendTelegramMessage(const String &message) {
  bot.sendMessage(CHAT_ID, message, "");
}

//  Alert logic 
void checkAndAlert(int percent) {
  if (percent <= CRITICAL_THRESHOLD) {
    sendTelegramMessage("🚨 CRITICAL ALERT! Feed level is " + String(percent) + "%. Order immediately!");
  } else if (percent <= LOW_LEVEL_THRESHOLD) {
    sendTelegramMessage("⚠️ WARNING: Feed level is " + String(percent) + "%. Time to reorder.");
  }
}

//  Connect to WiFi 
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nConnection failed");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  connectWiFi();
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  // 1. Take measurement
  float distance = getAveragedDistance();
  if (distance > 0) {
    int percent = calculateFillPercent(distance);
    Serial.println("Distance: " + String(distance) + " cm | Level: " + String(percent) + "%");
    
    sendTelegramMessage(" 2-Hour Report: Feed level is " + String(percent) + "%");
    checkAndAlert(percent);
  } else {
    Serial.println("Sensor read failed - check wiring");
    sendTelegramMessage("❌ Sensor read failed. Please check connections");
  }

  // 2. Disconnect WiFi to save power
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // 3. Go to deep sleep for 2 hours
  Serial.println("Entering Deep Sleep for 2 hours...");
  delay(1000); // Give time for message to be sent
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
  esp_deep_sleep_start();
}

void loop() {
  // Not used in deep sleep version
}
