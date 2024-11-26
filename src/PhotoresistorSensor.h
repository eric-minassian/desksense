#ifndef PHOTORESISTOR_SENSOR_H
#define PHOTORESISTOR_SENSOR_H

#include <Arduino.h>

#include "BaseSensor.h"

class PhotoresistorSensor : public BaseSensor {
 public:
  PhotoresistorSensor(uint8_t sensorPin) : analogPin(sensorPin) {}

  bool begin() override {
    // Initialize sensor
    pinMode(analogPin, INPUT);

    // Take a few readings to stabilize
    for (int i = 0; i < 5; i++) {
      measure();
      delay(10);
    }

    isInitialized = true;
    return true;
  }

  bool measure() override {
    if (!isInitialized) return false;

    // Take multiple samples to get a more stable reading
    long sum = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
      sum += analogRead(analogPin);
      delay(2);
    }

    // Calculate average
    currentRawValue = sum / SAMPLE_COUNT;

    // Map raw value to percentage (0 - 50) = (0% - 100%)
    currentLightPercent = map(currentRawValue, minReading, maxReading, 0, 100);

    // Update min/max values
    if (currentRawValue < minReading || minReading == -1)
      minReading = currentRawValue;
    if (currentRawValue > maxReading) maxReading = currentRawValue;

    return true;
  }

  void printMeasurements() override {
    if (!isInitialized) return;

    Serial.print("Light Level: ");
    Serial.print(currentLightPercent);
    Serial.print("% (Raw: ");
    Serial.print(currentRawValue);
    Serial.println(")");
  }

  void displayMeasurements(int& yPos) override {
    if (!isInitialized) return;

    char buffer[40];
    auto& display = DisplayManager::getInstance();

    snprintf(buffer, sizeof(buffer), "Light: %d%%", (int)currentLightPercent);
    display.drawText(buffer, 5, yPos);
    yPos += 30;
  }

  // Getters
  float getLightPercentage() const { return currentLightPercent; }
  int getRawValue() const { return currentRawValue; }

  // Optional: Get min/max readings for calibration
  int getMinReading() const { return minReading; }
  int getMaxReading() const { return maxReading; }

  // Reset min/max values
  void resetMinMax() {
    minReading = -1;
    maxReading = 0;
  }

  // Calibration
  void setThresholds(int darkThreshold, int brightThreshold) {
    this->darkThreshold = darkThreshold;
    this->brightThreshold = brightThreshold;
  }

  // Light level categories
  enum class LightLevel { DARK, DIM, BRIGHT };

  LightLevel getLightLevel() const {
    if (currentRawValue <= darkThreshold) {
      return LightLevel::DARK;
    } else if (currentRawValue >= brightThreshold) {
      return LightLevel::BRIGHT;
    }
    return LightLevel::DIM;
  }

 private:
  const uint8_t analogPin;
  const uint8_t SAMPLE_COUNT = 10;  // Number of samples to average

  int currentRawValue = 0;
  float currentLightPercent = 0.0f;

  int minReading = 0;
  int maxReading = 50;

  // Thresholds for light level categories (can be adjusted)
  int darkThreshold = 100;    // Readings below this are considered dark
  int brightThreshold = 900;  // Readings above this are considered bright

  float map(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
};

#endif