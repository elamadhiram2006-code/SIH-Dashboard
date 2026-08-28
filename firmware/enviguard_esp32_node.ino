/**
 * ==============================================================================
 * PROJECT: ENVIGUARD - AI-Powered Distributed Environmental Early Warning Network
 * PROBLEM STATEMENT: SIH26178 (Smart India Hackathon 2026)
 * MODULE: ESP32 Multi-Sensor Telemetry & Local Warning Node Firmware
 * ==============================================================================
 * 
 * SENSORS CONNECTED:
 * 1. DHT22 (Temp & Humidity)          -> GPIO 4
 * 2. MQ-2 (Smoke / Gas)               -> GPIO 34 (ADC1_CH6 via divider)
 * 3. MQ-135 (Air Quality / Volatiles) -> GPIO 35 (ADC1_CH7 via divider)
 * 4. HC-SR04 Ultrasonic Trigger      -> GPIO 18
 * 5. HC-SR04 Ultrasonic Echo         -> GPIO 19
 * 6. Raindrop Conductive Sensor       -> GPIO 23
 * 7. IR Optical Flame Sensor          -> GPIO 25
 * 8. SSD1306 OLED (I2C)               -> SDA: GPIO 21, SCL: GPIO 22
 * 9. Piezo Warning Buzzer             -> GPIO 27
 * 10. Status LEDs (Red, Amber, Green) -> GPIO 12, 14, 26
 * 
 * NOTE ON PROTOTYPE ACCURACY:
 * - MQ-2 and MQ-135 outputs are processed as normalized indicative signals.
 * - Multi-parameter heuristic risk evaluation is computed locally before dispatching
 *   structured JSON telemetry to the ENVIGUARD command center.
 * ==============================================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// Wi-Fi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ENVIGUARD Ingestion Gateway Endpoint
const char* serverEndpoint = "http://192.168.1.100:3001/api/telemetry";

// Node Configuration
#define NODE_ID "NODE-01"
#define ZONE_NAME "Forest Zone A"

// Pin Definitions
#define DHTPIN 4
#define DHTTYPE DHT22
#define PIN_MQ2 34
#define PIN_MQ135 35
#define PIN_TRIG 18
#define PIN_ECHO 19
#define PIN_RAIN 23
#define PIN_FLAME 25
#define PIN_BUZZER 27
#define PIN_LED_RED 12
#define PIN_LED_AMBER 14
#define PIN_LED_GREEN 26

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DHT dht(DHTPIN, DHTTYPE);

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 3000; // Sample and send every 3 seconds

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[ENVIGUARD] Booting Multi-Sensor Node: " NODE_ID);

  // Initialize GPIO Modes
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_RAIN, INPUT);
  pinMode(PIN_FLAME, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_AMBER, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  digitalWrite(PIN_LED_GREEN, HIGH);

  // Initialize Sensors & Display
  dht.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[WARN] SSD1306 OLED allocation failed");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("ENVIGUARD SIH26178");
    display.setCursor(0, 25);
    display.println("Connecting Wi-Fi...");
    display.display();
  }

  // Connect Wi-Fi
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[Wi-Fi] Connected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[Wi-Fi] Operating in Local Standalone / Ad-hoc Mode");
  }
}

float measureUltrasonicWaterLevel() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 25000); // 25ms timeout
  if (duration == 0) return 0.25; // Default baseline fallback

  float distanceCm = duration * 0.034 / 2.0;
  // Convert overhead sensor distance to estimated water surface level (assuming 3.0m mounting height)
  float waterDepthMeters = (300.0 - distanceCm) / 100.0;
  if (waterDepthMeters < 0) waterDepthMeters = 0.0;
  return waterDepthMeters;
}

void loop() {
  if (millis() - lastSendTime >= sendInterval) {
    lastSendTime = millis();

    // 1. Read Physical Sensors
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (isnan(temperature)) temperature = 29.2;
    if (isnan(humidity)) humidity = 52.0;

    int rawSmoke = analogRead(PIN_MQ2);       // 12-bit ADC (0-4095) scaled to normalized 0-1023
    int smokeLevel = rawSmoke / 4;

    int rawAQ = analogRead(PIN_MQ135);
    int airQualityLevel = rawAQ / 4;

    float waterLevel = measureUltrasonicWaterLevel();
    bool rainDetected = (digitalRead(PIN_RAIN) == LOW);
    bool flameDetected = (digitalRead(PIN_FLAME) == LOW);

    // 2. Multi-Sensor Local Risk Assessment
    bool isCritical = (flameDetected && (smokeLevel > 400 || temperature > 38.0)) ||
                      (waterLevel > 2.10) ||
                      (airQualityLevel > 750);

    bool isWarning = (temperature > 38.0 && humidity < 30.0 && smokeLevel > 350) ||
                     (rainDetected && waterLevel > 1.30) ||
                     (airQualityLevel > 480);

    // Update Local Visual / Audible Indicators
    if (isCritical) {
      digitalWrite(PIN_LED_RED, HIGH);
      digitalWrite(PIN_LED_AMBER, LOW);
      digitalWrite(PIN_LED_GREEN, LOW);
      tone(PIN_BUZZER, 2400, 200); // Pulsed alarm siren
    } else if (isWarning) {
      digitalWrite(PIN_LED_RED, LOW);
      digitalWrite(PIN_LED_AMBER, HIGH);
      digitalWrite(PIN_LED_GREEN, LOW);
      noTone(PIN_BUZZER);
    } else {
      digitalWrite(PIN_LED_RED, LOW);
      digitalWrite(PIN_LED_AMBER, LOW);
      digitalWrite(PIN_LED_GREEN, HIGH);
      noTone(PIN_BUZZER);
    }

    // 3. Update OLED Display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("NODE: %s\n", NODE_ID);
    display.printf("T:%.1fC H:%.0f%% W:%.2fm\n", temperature, humidity, waterLevel);
    display.printf("Smoke:%d AQ:%d\n", smokeLevel, airQualityLevel);
    display.printf("Status: %s\n", isCritical ? "CRITICAL!" : isWarning ? "WARNING" : "NORMAL");
    display.display();

    // 4. Construct JSON Payload
    StaticJsonDocument<300> doc;
    doc["node_id"] = NODE_ID;
    doc["zone"] = ZONE_NAME;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["smoke_level"] = smokeLevel;
    doc["air_quality_level"] = airQualityLevel;
    doc["water_level"] = waterLevel;
    doc["rain_detected"] = rainDetected;
    doc["flame_detected"] = flameDetected;

    String jsonString;
    serializeJson(doc, jsonString);

    // 5. Dispatch over Wi-Fi HTTP POST to Gateway
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverEndpoint);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonString);
      if (httpResponseCode > 0) {
        Serial.printf("[HTTP] Telemetry POST code: %d\n", httpResponseCode);
      } else {
        Serial.printf("[HTTP] Error sending telemetry: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    } else {
      Serial.println("[SERIAL LOG] " + jsonString);
    }
  }
}
