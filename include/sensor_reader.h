#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <Arduino.h>

struct SensorReading {
  float temperature;
  float humidity;
  bool valid;
  unsigned long updatedAt;
};

void sensor_init(void);
void sensor_update(void);
SensorReading sensor_reading(void);

#endif