#include "sensor_reader.h"

#include <ModbusMaster.h>
#include <Preferences.h>

constexpr uint8_t kRxPin = 18;
constexpr uint8_t kTxPin = 17;
HardwareSerial modbusSerial(2);
ModbusMaster modbusNode;

namespace {
SensorReading reading = {NAN, NAN, false, 0};
unsigned long lastRead = 0;
constexpr unsigned long kReadInterval = 2000;
uint8_t deviceId = 1;
}

void sensor_init(void) {
  Preferences preferences;
  preferences.begin("aqualab", true);
  deviceId = preferences.getUChar("device_id", 1);
  preferences.end();
  if (deviceId < kSensorDeviceIdMin || deviceId > kSensorDeviceIdMax) deviceId = 1;

  modbusSerial.begin(9600, SERIAL_8N1, kRxPin, kTxPin);
  modbusNode.begin(deviceId, modbusSerial);
  Serial.printf("[Sensor] Backend Modbus | RX18 TX17 | Slave %d\n", deviceId);
}

void sensor_update(void) {
  if (millis() - lastRead < kReadInterval) return;
  lastRead = millis();

  const uint8_t result = modbusNode.readInputRegisters(0x0000, 2);
  if (result != modbusNode.ku8MBSuccess) {
    reading.valid = false;
    Serial.printf("[Sensor] Modbus gagal: 0x%02X\n", result);
    return;
  }
  const uint16_t rawTemp = modbusNode.getResponseBuffer(0);
  const uint16_t rawHumidity = modbusNode.getResponseBuffer(1);
  float temperature = rawTemp / 100.0f;
  if (rawTemp > 0x7FFF) temperature = (rawTemp - 0x10000) / 100.0f;
  reading = {temperature, rawHumidity / 100.0f, true, millis()};

  Serial.printf("[Sensor] Suhu: %.2f C | Kelembapan: %.2f %%RH\n",
                reading.temperature, reading.humidity);
}

SensorReading sensor_reading(void) { return reading; }

bool sensor_set_device_id(uint8_t id) {
  if (id < kSensorDeviceIdMin || id > kSensorDeviceIdMax) return false;
  deviceId = id;
  Preferences preferences;
  preferences.begin("aqualab", false);
  preferences.putUChar("device_id", deviceId);
  preferences.end();
  modbusNode.begin(deviceId, modbusSerial);
  reading.valid = false;
  Serial.printf("[Sensor] Device ID diganti ke %d\n", deviceId);
  return true;
}

uint8_t sensor_get_device_id(void) { return deviceId; }