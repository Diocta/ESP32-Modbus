#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <Arduino.h>

struct SensorReading {
  float temperature;
  float humidity;
  bool valid;
  unsigned long updatedAt;
};

constexpr uint8_t kSensorDeviceIdMin = 1;
constexpr uint8_t kSensorDeviceIdMax = 3;

void sensor_init(void);
void sensor_update(void);
SensorReading sensor_reading(void);

// ID slave Modbus sensor (harus sama dengan address SW1 pada sensor fisik).
bool sensor_set_device_id(uint8_t id);
uint8_t sensor_get_device_id(void);

#endif