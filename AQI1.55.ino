#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <math.h>
#include <HardwareSerial.h>

HardwareSerial MySerial(2); // UART2

// WiFi credentials
const char* WIFI_SSID     = "WIFI NMAE";
const char* WIFI_PASSWORD = "WIFI PASSWORD";

// Firebase
const char* DATABASE_URL = "URL";
const char* DB_AUTH = ""; 

// Sensor pins
#define PM25_PIN 17
#define PM10_PIN 16
#define CO2_PIN 32
#define MQ135_PIN 34 
#define DHTPIN 4 
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE); 

float RL = 10.0;  // kΩ
float R0 = 10.0;  

// --- Helper functions ---
float getResistance(int rawADC) {
  return ((4095.0 / rawADC - 1.0) * RL);
}

float getPPM(float ratio, float a, float b) {
  return pow(10, ((log10(ratio) - b) / a));
}

float ppmToUg(float ppm, float MW) {
  return ppm * (MW / 24.45); 
}

int readPM25() {
  int raw = analogRead(PM25_PIN); 
  return map(raw, 0, 4095, 0, 150);
}

int readPM10() {
  int raw = analogRead(PM10_PIN); 
  return map(raw, 0, 4095, 0, 200);
}

int readCO2() {
  int raw = analogRead(CO2_PIN); 
  return map(raw, 0, 4095, 350, 2000);
}

int readMQ135() {
  int raw = analogRead(MQ135_PIN); 
  return map(raw, 0, 4095, 0, 1000);
}

float readTemperature() {
  float t = dht.readTemperature();
  return isnan(t) ? -1 : t;
}

float readHumidity() {
  float h = dht.readHumidity();
  return isnan(h) ? -1 : h;
}

//  WiFi & Firebase 
void connectWiFi() {
  Serial.print("Connecting to WiFi ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    if (millis() - start > 15000) {
      Serial.println("\nWiFi connect timeout, retrying...");
      start = millis();
    }
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

bool sendToFirebase(const String &path, const String &jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient https;
  String url = String(DATABASE_URL) + String(path) + ".json" + String(DB_AUTH);
  https.begin(url);
  https.addHeader("Content-Type", "application/json");

  int httpCode = https.PUT(jsonPayload);
  if (httpCode > 0) {
    String payload = https.getString();
    Serial.printf("Firebase %s -> code: %d, resp: %s\n", url.c_str(), httpCode, payload.c_str());
    https.end();
    return (httpCode >= 200 && httpCode < 300);
  } else {
    Serial.printf("HTTP request failed, error: %s\n", https.errorToString(httpCode).c_str());
    https.end();
    return false;
  }
}

// Setup 
void setup() {
  Serial.begin(115200);
  delay(100);
  MySerial.begin(9600, SERIAL_8N1, 17, 16); // RX=17, TX=16

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  connectWiFi();
  dht.begin();
}

//  Main loop 
void loop() {
  // Read sensors
  int pm25 = readPM25()+0.3;
  int pm10 = readPM10()+0.5;
  int co2  = readCO2();
  int mq135 = readMQ135();
  float temperature = readTemperature();
  float humidity = readHumidity() - 8; // optional offset

  // Calculate NO2 from MQ135
  int adcMQ = analogRead(MQ135_PIN);
  float RS = getResistance(adcMQ);
  float ratio = RS / R0;
  float ppmNO2 = getPPM(ratio, -0.8, 0.9);
  float ugNO2 = ppmToUg(ppmNO2, 46.0);

  // Timestamp
  unsigned long ts = millis();

  // Serial debug
  Serial.printf("PM2.5: %d | PM10: %d | CO2: %d | NOx: %.2f | mq135: %d | Temp: %.1f°C | Hum: %.1f%%\n",
                pm25, pm10, co2, ugNO2, mq135, temperature, humidity);

  // UART2 output (only PM2.5, PM10, CO2)
  MySerial.printf("PM2.5:%d µg/m³ | PM10:%d µg/m³ | CO2:%d ppm\n", pm25, pm10, co2);

  // Firebase JSON
  StaticJsonDocument<256> doc;
  doc["pm25"] = pm25;
  doc["pm10"] = pm10;
  doc["co2"] = co2;
  doc["nox"] = ugNO2;
  doc["mq135"] = mq135;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["timestamp_ms"] = ts;

  String json;
  serializeJson(doc, json);
  bool ok = sendToFirebase("/sensors/node1", json);
  if (!ok) Serial.println("Failed to write to Firebase");

  delay(5000); // 5-second sampling
}
