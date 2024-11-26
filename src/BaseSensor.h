#ifndef BASE_SENSOR_H
#define BASE_SENSOR_H

class BaseSensor {
 public:
  virtual ~BaseSensor() = default;
  virtual bool begin() = 0;
  virtual bool measure() = 0;
  virtual void printMeasurements() = 0;

 protected:
  bool isInitialized = false;
};

#endif