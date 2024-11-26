#include "DHT20Sensor.h"
#include "MAX9814.h"
#include "PhotoresistorSensor.h"
#include "SGP30Sensor.h"

SGP30Sensor airSensor;
DHT20Sensor tempSensor;
MAX9814Sensor micSensor(2);
PhotoresistorSensor lightSensor(15);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }

  Serial.println("SGP30 test");

  if (!airSensor.begin()) {
    Serial.println("Sensor not found :(");
    while (1);
  }

  // Optional: Set baseline if available
  // airSensor.setBaseline(0x8E68, 0x8F41);

  Serial.println("Found SGP30");

  Serial.println("DHT20 test");

  if (!tempSensor.begin()) {
    Serial.println("Sensor not found :(");
    while (1);
  }

  Serial.println("Found DHT20");

  Serial.println("MAX9814 test");

  if (!micSensor.begin()) {
    Serial.println("Sensor not found :(");
    while (1);
  }

  // Optional: Set decibel range for calibration
  micSensor.setDbRange(35.0, 80.0);

  Serial.println("Found MAX9814");

  Serial.println("Photoresistor test");

  if (!lightSensor.begin()) {
    Serial.println("Sensor not found :(");
    while (1);
  }

  // lightSensor.setThresholds(50, 500);

  Serial.println("Found Photoresistor");
}

void loop() {
  if (!airSensor.measure()) {
    Serial.println("Measurement failed");
    return;
  }

  airSensor.printMeasurements();

  if (!tempSensor.measure()) {
    Serial.println("Measurement failed");
    return;
  }

  tempSensor.printMeasurements();

  if (!micSensor.measure()) {
    Serial.println("Measurement failed");
    return;
  }

  micSensor.printMeasurements();

  if (!lightSensor.measure()) {
    Serial.println("Measurement failed");
    return;
  }

  lightSensor.printMeasurements();

  delay(1000);

  // Optional: Baseline monitoring example
  /*
  static int counter = 0;
  if (++counter >= 30) {
      counter = 0;
      uint16_t TVOC_base, eCO2_base;
      if (airSensor.getBaseline(&eCO2_base, &TVOC_base)) {
          Serial.print("****Baseline values: eCO2: 0x");
          Serial.print(eCO2_base, HEX);
          Serial.print(" & TVOC: 0x");
          Serial.println(TVOC_base, HEX);
      }
  }
  */
}