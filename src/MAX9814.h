#ifndef MAX9814_SENSOR_H
#define MAX9814_SENSOR_H

#include <Arduino.h>

#include "BaseSensor.h"

class MAX9814Sensor : public BaseSensor {
 public:
  MAX9814Sensor(uint8_t outputPin) : analogPin(outputPin) {}

  bool begin() override {
    pinMode(analogPin, INPUT);

    isInitialized = true;
    return true;
  }

  bool measure() override {
    if (!isInitialized) return false;

    // Take multiple samples to get a more stable reading
    long sum = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
      int16_t sample = analogRead(analogPin);
      sum += abs(sample - DC_OFFSET);
      delay(1);  // Short delay between samples
    }

    // Calculate average
    float average = sum / SAMPLE_COUNT;

    // Convert to decibels (approximate)
    // Note: This needs calibration for accurate readings
    currentDb = map(average, 0, maxAnalogValue(), DB_MIN, DB_MAX);

    return true;
  }

  void printMeasurements() override {
    if (!isInitialized) return;

    Serial.print("Sound Level: ");
    Serial.print(currentDb);
    Serial.println(" dB");
  }

  void displayMeasurements(int& yPos) override {
    if (!isInitialized) return;

    char buffer[40];
    auto& display = DisplayManager::getInstance();

    snprintf(buffer, sizeof(buffer), "Sound: %.1f dB", currentDb);
    display.drawText(buffer, 5, yPos);
    yPos += 30;
  }

  // Getter
  float getDecibels() const { return currentDb; }

  // Calibration methods
  void setDbRange(float minDb, float maxDb) {
    DB_MIN = minDb;
    DB_MAX = maxDb;
  }

 private:
  const uint8_t analogPin;
  const uint16_t SAMPLE_COUNT = 100;  // Number of samples to average
  const uint16_t DC_OFFSET = 512;     // Midpoint of analog reading

  float currentDb = 0.0f;
  float DB_MIN = 48.0f;  // Minimum dB value (adjust based on calibration)
  float DB_MAX = 90.0f;  // Maximum dB value (adjust based on calibration)

  // Helper function to get maximum analog value based on board's ADC resolution
  int maxAnalogValue() {
#if defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_ESP32)
    return 4095;  // 12-bit ADC
#else
    return 1023;  // 10-bit ADC
#endif
  }

  float map(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
};

#endif