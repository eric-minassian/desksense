#ifndef DHT20_SENSOR_H
#define DHT20_SENSOR_H

#include <Adafruit_AHTX0.h>
#include <Arduino.h>

#include "BaseSensor.h"

class DHT20Sensor : public BaseSensor {
 public:
  DHT20Sensor() = default;

  bool begin() override {
    if (!aht.begin()) {
      return false;
    }
    isInitialized = true;
    return true;
  }

  bool measure() override {
    if (!isInitialized) return false;

    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    currentTemp = temp.temperature;
    currentTempF = celsiusToFahrenheit(currentTemp);
    currentHumidity = humidity.relative_humidity;

    return true;
  }

  void printMeasurements() override {
    if (!isInitialized) return;

    Serial.print("Temperature: ");
    Serial.print(currentTemp);
    Serial.print(" °C / ");
    Serial.print(currentTempF);
    Serial.println(" °F");

    Serial.print("Humidity: ");
    Serial.print(currentHumidity);
    Serial.println(" RH %");
  }

  void displayMeasurements(int& yPos) override {
    if (!isInitialized) return;

    char buffer[40];
    auto& display = DisplayManager::getInstance();

    snprintf(buffer, sizeof(buffer), "Temp: %.1fC/%.1fF", currentTemp,
             currentTempF);
    display.drawText(buffer, 5, yPos);
    yPos += 30;

    snprintf(buffer, sizeof(buffer), "Humidity: %.1f%%", currentHumidity);
    display.drawText(buffer, 5, yPos);
    yPos += 30;
  }

  // Getters
  float getTemperatureCelsius() const { return currentTemp; }
  float getTemperatureFahrenheit() const { return currentTempF; }
  float getHumidity() const { return currentHumidity; }

 private:
  Adafruit_AHTX0 aht;
  float currentTemp = 0.0f;    // Celsius
  float currentTempF = 32.0f;  // Fahrenheit
  float currentHumidity = 0.0f;

  // Utility function for temperature conversion
  static float celsiusToFahrenheit(float celsius) {
    return (celsius * 9.0f / 5.0f) + 32.0f;
  }
};

#endif