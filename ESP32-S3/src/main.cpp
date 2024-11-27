#define BLYNK_TEMPLATE_ID "TMPL6001DbDLu"
#define BLYNK_TEMPLATE_NAME "ESP32 S3"
#define BLYNK_AUTH_TOKEN "4972uUIJGD87GGsYVzQvWniwAYwGLOve"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Arduino.h>
#include <BH1750FVI.h>

char ssid[] = "pimandpie_2G";
char pass[] = "n1l2p3p4";

// Define pins and sensors
#define RX (35)
#define TX (36)
#define touchSensorPin (20)
#define relayPin (21)
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);

// Define variable to record recieved data from ESP32 DEVKIT V1
String receivedData;

// Define Variable to record data from sensors
float humidity, temperature;
int gasData;
uint16_t lux;
int motionState = LOW;

// Debounce time 1.5 s
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 750;

// Extract recieved data from ESP32 DEVKIT V1
void processReceivedData(String data) {
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
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, temperature);
  Blynk.virtualWrite(V3, gasData);
  Blynk.virtualWrite(V4, lux);
}

void setup() {
  pinMode(touchSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX, TX);

  LightSensor.begin();  

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();

  // Read data from sensor node (ESP32 DEVKIT V1)
  if (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      processReceivedData(receivedData);
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
      }
      lastDebounceTime = millis();
    }
  }
}