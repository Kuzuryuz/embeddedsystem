// Define Blynk's variables
#define BLYNK_TEMPLATE_ID "TMPL6vPpIktZS"
#define BLYNK_TEMPLATE_NAME "EmbeddedProject"
#define BLYNK_AUTH_TOKEN "sk0yCLU1jZHmNOZOXWDqfsimqR1eeSly"

#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <Firebase_ESP_Client.h>
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"
#include <BH1750FVI.h>

// Define WiFi name and password
char ssid[] = "";
char pass[] = "";

// Define Firebase's variables
#define API_KEY "AIzaSyBzfFM6Z7fJ2OSBFci0oCO0YSU-UZvDohw"
#define DATABASE_URL "https://embedded-project-7589a-default-rtdb.asia-southeast1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

const char* lineToken = "Iu2HRiaTEBV6IN7V13tjkcQ1rwQzV72TDynPXG6LhV1";

// Define pins and sensors
#define RX1D (16)
#define TX1D (17)
#define RX2D (5)
#define TX2D (18)
#define touchSensorPin (2)
#define relayPin (4)
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);

// Define variable to store data from sensor and sensor node
String receivedData;
float humidity, temperature;
int gasData;
uint16_t lux;
int motionState = LOW;

// Line notification delay time
unsigned long lastTempAlarm = 0;
unsigned long lastSmokeAlarm = 0;
unsigned long alarmDelay = 30000;

// Button and voice control debounce time
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
    Serial.println("------------------------------------------------------------");

    // Send data to cloud platform
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, temperature);
    Blynk.virtualWrite(V3, gasData);
    Blynk.virtualWrite(V4, lux);

    if (Firebase.ready() && signupOK) {
      Serial.println("Send data to Firebase...");
      Firebase.RTDB.setFloat(&fbdo, "Sensor/Humidity", humidity);
      Firebase.RTDB.setFloat(&fbdo, "Sensor/Temperature", temperature);
      Firebase.RTDB.setInt(&fbdo, "Sensor/GasLevel", gasData);
      Firebase.RTDB.setInt(&fbdo, "Sensor/LightLevel", lux);
      Serial.println("Sent complete!");
      Serial.println("------------------------------------------------------------");
    }
  } else if (data.startsWith("command_id")) {
    if (millis() - lastDebounceTime > debounceDelay) {
      int index = data.indexOf(":") + 2;
      int command_id = data.substring(index).toInt();

      // Control the LED
      if (command_id == 13) {
        digitalWrite(relayPin, HIGH);
        Serial.println("Voice Command Detected! Turning LED on");
        Serial.println("------------------------------------------------------------");
      } else if (command_id == 14) {
        digitalWrite(relayPin, LOW);
        Serial.println("Voice Command Detected! Turning LED off");
        Serial.println("------------------------------------------------------------");
      }

      // Send data to cloud platform
      Blynk.virtualWrite(V0, digitalRead(relayPin));

      if (Firebase.ready() && signupOK){
        Firebase.RTDB.setBool(&fbdo, "Actuator/LED", digitalRead(relayPin));
      }

      // Reset debouce time
      lastDebounceTime = millis();
    }
  }

  // If high temperature and smoke detection
  String notiMessage;

  // Check if high temperature
  if (temperature > 35.00 && (millis() - lastTempAlarm > alarmDelay)) {
    Serial.println("High Temperature Detected! (" + String(temperature) + "°C)");
    notiMessage = "ตรวจพบอุณหภูมิสูง (" + String(temperature) + "°C)";
    sendLineNotify(notiMessage);

    lastTempAlarm = millis();
  }

  // Check if high gas level
  if (gasData > 500 && (millis() - lastSmokeAlarm > alarmDelay)) {
    Serial.println("High Gas Level Detected! (" + String(gasData) + "ppm)");
    notiMessage = "ตรวจพบควันปริมาณมาก (" + String(gasData) + " ppm)";
    sendLineNotify(notiMessage);

    lastSmokeAlarm = millis();
  }
}

void setup() {
  pinMode(touchSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX1D, TX1D);
  Serial2.begin(115200, SERIAL_8N1, RX2D, TX2D);

  LightSensor.begin();  

  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to WiFi!");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Edit Firebase's configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up Firebase
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  
  // Assign the callback function for the long running token generation task 
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  Blynk.run();

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

  // Read data from Microphone module (ESP32-S3)
  if (Serial2.available()) {
    char c = Serial2.read();
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
          Serial.println("Touch Sensor Detected! Turning LED on");
          Serial.println("------------------------------------------------------------");
        } else {
          Serial.println("Touch Sensor Detected! Turning LED off");
          Serial.println("------------------------------------------------------------");
        }
        digitalWrite(relayPin, !digitalRead(relayPin));

        // Send data to cloud platform
        Blynk.virtualWrite(V0, digitalRead(relayPin));

        if (Firebase.ready() && signupOK){
          Firebase.RTDB.setBool(&fbdo, "Actuator/LED", digitalRead(relayPin));
        }
      }
      lastDebounceTime = millis();
    }
  }
}