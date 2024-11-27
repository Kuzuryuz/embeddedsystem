#include "DHT.h"

#define sensorPin 2
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define RX (16)
#define TX (17)

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX, TX);
  dht.begin();
}

void loop() {
  // Read data from DHT22
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Read analog data from the smoke and gas sensor
  int avalue = analogRead(sensorPin);

  // Concat data
  String data = "Humidity: " + String(h, 2) + "% | Temperature: " + String(t, 2) + "°C | Smoke and gas: " + String(avalue);

  // Send data to ESP32-S3
  Serial1.println(data);

  // Print data sended
  Serial.println("Sent to ESP32-S3: ");
  Serial.println(data);
  Serial.println("------------------------------------------------------------");

  delay(1000);
}