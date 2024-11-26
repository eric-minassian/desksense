#ifndef SGP30_SENSOR_H
#define SGP30_SENSOR_H

#include <Wire.h>

#include "Adafruit_SGP30.h"
#include "BaseSensor.h"

class SGP30Sensor : public BaseSensor {
 public:
  SGP30Sensor() = default;

  bool begin() override {
    if (!sgp.begin()) {
      return false;
    }
    isInitialized = true;
    return true;
  }

  bool measure() override {
    if (!isInitialized) return false;

    bool success = true;
    success &= measureIAQ();
    success &= measureRaw();
    return success;
  }

  void printMeasurements() override {
    if (!isInitialized) return;

    printIAQMeasurements();
    printRawMeasurements();
  }

  void displayMeasurements(int& yPos) override {
    if (!isInitialized) return;

    char buffer[40];
    auto& display = DisplayManager::getInstance();

    snprintf(buffer, sizeof(buffer), "TVOC: %d ppb", sgp.TVOC);
    display.drawText(buffer, 5, yPos);
    yPos += 30;

    snprintf(buffer, sizeof(buffer), "eCO2: %d ppm", sgp.eCO2);
    display.drawText(buffer, 5, yPos);
    yPos += 30;
  }

  // Getters for sensor values
  uint16_t getTVOC() const { return sgp.TVOC; }
  uint16_t geteCO2() const { return sgp.eCO2; }
  uint16_t getRawH2() const { return sgp.rawH2; }
  uint16_t getRawEthanol() const { return sgp.rawEthanol; }

  // Optional: Set baseline for calibration
  void setBaseline(uint16_t eco2_base, uint16_t tvoc_base) {
    if (isInitialized) {
      sgp.setIAQBaseline(eco2_base, tvoc_base);
    }
  }

  // Optional: Get baseline readings
  bool getBaseline(uint16_t* eco2_base, uint16_t* tvoc_base) {
    if (!isInitialized) return false;
    return sgp.getIAQBaseline(eco2_base, tvoc_base);
  }

 private:
  Adafruit_SGP30 sgp;

  bool measureIAQ() { return sgp.IAQmeasure(); }

  bool measureRaw() { return sgp.IAQmeasureRaw(); }

  void printIAQMeasurements() {
    Serial.print("TVOC ");
    Serial.print(sgp.TVOC);
    Serial.print(" ppb\t");
    Serial.print("eCO2 ");
    Serial.print(sgp.eCO2);
    Serial.println(" ppm");
  }

  void printRawMeasurements() {
    Serial.print("Raw H2 ");
    Serial.print(sgp.rawH2);
    Serial.print(" \t");
    Serial.print("Raw Ethanol ");
    Serial.print(sgp.rawEthanol);
    Serial.println("");
  }
};

#endif