// #define BLYNK_TEMPLATE_ID "TMPL6lEgwGCl1"
// #define BLYNK_TEMPLATE_NAME "Project"
// #define BLYNK_AUTH_TOKEN "cGFTkOIASPTcT4cgfxL6jOWcRcMiP8ZX"

// #define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
// #include <BlynkSimpleEsp32.h>
#include <Arduino.h>
#include <BH1750FVI.h>

char ssid[] = "";
char pass[] = "";

const char* lineToken = "EtXxXHJfnpg0UVtf5oj1X9sZkMTuoj7UtYNk9z2hAPP";

// Define pins and sensors
#define RX (35)
#define TX (36)
#define touchSensorPin (20)
#define relayPin (21)
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);

// Define variable to store data from sensor and sensor node
String receivedData;
float humidity, temperature;
int gasData;
uint16_t lux;
int motionState = LOW;

// Debounce time 1.5 s
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 750;

// Function to send notification to line
void sendLineNotify(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://notify-api.line.me/api/notify";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Bearer " + String(lineToken));
    String body = "message=" + String(message);
    int httpResponseCode = http.POST(body);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response Code: ");
      Serial.println(httpResponseCode);
      Serial.println(http.getString());
      Serial.println("------------------------------------------------------------");
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      Serial.println("------------------------------------------------------------");
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

// Extract recieved data from ESP32 DEVKIT V1
void sendDataToCloudPlatform(String data) {
  if (data.startsWith("Humidity:")) {
    // Get humidity data
    int hIndex = data.indexOf(':') + 2;
    humidity = data.substring(hIndex, hIndex + 5).toFloat();

    // Get temperature data
    int tIndex = data.indexOf("Temperature:") + 13;
    temperature = data.substring(tIndex, tIndex + 5).toFloat();

    // Get smoke and gas data
    int gIndex = data.indexOf("Smoke and gas:") + 15;
    gasData = data.substring(gIndex).toInt();

    // Get light level from BH1750FVI
    lux = LightSensor.GetLightIntensity();

    // Print data
    Serial.print("Humidity: " + String(humidity));
    Serial.print(" | Temperature: " + String(temperature));
    Serial.print(" | Gas: " + String(gasData));
    Serial.println(" | Light: " + String(lux));
  }

  // Send to Blynk
  // Blynk.virtualWrite(V1, humidity);
  // Blynk.virtualWrite(V2, temperature);
  // Blynk.virtualWrite(V3, gasData);
  // Blynk.virtualWrite(V4, lux);

  // If high temperature and smoke detection
  if (temperature > 45.00) {
    Serial.println("------------------------------------------------------------");
    Serial.println("Fire Alarm Detected!");
    String notiMessage = "ตรวจพบควันและอุณหภูมิสูง (" + String(temperature) + "°C)";
    sendLineNotify(notiMessage);
  }
}

void setup() {
  pinMode(touchSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX, TX);

  LightSensor.begin();  

  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to WiFi!");

  // Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  // Blynk.run();

  // Read data from sensor node (ESP32 DEVKIT V1)
  if (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      sendDataToCloudPlatform(receivedData);
      receivedData = "";
    } else {
      receivedData += c;
    }
  }

  // Detect touch sensor with debouncing time
  int reading = digitalRead(touchSensorPin);
  if (reading != motionState) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      motionState = reading;
      if (motionState == HIGH) {
        if (digitalRead(relayPin) == LOW) {
          Serial.println("------------------------------------------------------------");
          Serial.println("Touch Sensor Detected! Turning LED on");
          Serial.println("------------------------------------------------------------");
        } else {
          Serial.println("------------------------------------------------------------");
          Serial.println("Touch Sensor Detected! Turning LED off");
          Serial.println("------------------------------------------------------------");
        }
        digitalWrite(relayPin, !digitalRead(relayPin));
        // Blynk.virtualWrite(V0, digitalRead(relayPin));
      }
      lastDebounceTime = millis();
    }
  }
}