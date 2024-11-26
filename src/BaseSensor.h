#ifndef BASE_SENSOR_H
#define BASE_SENSOR_H

#include "DisplayManager.h"

class BaseSensor {
 public:
  virtual ~BaseSensor() = default;
  virtual bool begin() = 0;
  virtual bool measure() = 0;
  virtual void printMeasurements() = 0;
  virtual void displayMeasurements(int& yPos) = 0;  // New method

 protected:
  bool isInitialized = false;
};

#endif