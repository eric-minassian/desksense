#include "DHT20Sensor.h"
#include "DisplayManager.h"
#include "MAX9814.h"
#include "PhotoresistorSensor.h"
#include "SGP30Sensor.h"

SGP30Sensor airSensor;
DHT20Sensor tempSensor;
MAX9814Sensor micSensor(2);
PhotoresistorSensor lightSensor(12);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }

  auto& display = DisplayManager::getInstance();
  display.begin();

  Serial.println("Attempting to find SGP30 sensor...");
  if (!airSensor.begin()) {
    while (1) {
      Serial.println("Could not find SGP30 sensor. Please check wiring.");
      delay(5000);
    }
  }
  Serial.println("Found SGP30 sensor");

  Serial.println("Attempting to find DHT20 sensor...");
  if (!tempSensor.begin()) {
    while (1) {
      Serial.println("Could not find DHT20 sensor. Please check wiring.");
      delay(5000);
    }
  }
  Serial.println("Found DHT20 sensor");

  Serial.println("Attempting to find MAX9814 sensor...");
  if (!micSensor.begin()) {
    while (1) {
      Serial.println("Could not find MAX9814 sensor. Please check wiring.");
      delay(5000);
    }
  }
  Serial.println("Found MAX9814 sensor");
  micSensor.setDbRange(35.0, 80.0);

  Serial.println("Attempting to find Photoresistor sensor...");
  if (!lightSensor.begin()) {
    while (1) {
      Serial.println(
          "Could not find Photoresistor sensor. Please check wiring.");
      delay(5000);
    }
  }
  Serial.println("Found Photoresistor sensor");
  lightSensor.setThresholds(10, 40);
}

void loop() {
  auto& display = DisplayManager::getInstance();
  display.clearScreen();

  int yPos = 5;  // Start a bit higher up

  if (tempSensor.measure()) {
    tempSensor.printMeasurements();
    tempSensor.displayMeasurements(yPos);
  }

  if (airSensor.measure()) {
    airSensor.printMeasurements();
    airSensor.displayMeasurements(yPos);
  }

  if (micSensor.measure()) {
    micSensor.printMeasurements();
    micSensor.displayMeasurements(yPos);
  }

  if (lightSensor.measure()) {
    lightSensor.printMeasurements();
    lightSensor.displayMeasurements(yPos);
  }

  display.pushToDisplay();
  delay(1000);
}