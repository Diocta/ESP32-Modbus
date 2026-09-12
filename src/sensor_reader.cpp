#include "sensor_reader.h"

#if defined(SENSOR_BACKEND_DHT11)
#include <DHT.h>
constexpr uint8_t kDhtPin = 4;
constexpr uint8_t kDhtType = DHT11;
DHT dht(kDhtPin, kDhtType);
#else
#include <ModbusMaster.h>
constexpr uint8_t kRxPin = 18;
constexpr uint8_t kTxPin = 17;
constexpr uint8_t kSlaveId = 1;
HardwareSerial modbusSerial(2);
ModbusMaster modbusNode;
#endif

namespace {
SensorReading reading = {NAN, NAN, false, 0};
unsigned long lastRead = 0;
constexpr unsigned long kReadInterval = 2000;
}

void sensor_init(void) {
#if defined(SENSOR_BACKEND_DHT11)
  dht.begin();
  Serial.println("[Sensor] Backend DHT11 | GPIO4");
#else
  modbusSerial.begin(9600, SERIAL_8N1, kRxPin, kTxPin);
  modbusNode.begin(kSlaveId, modbusSerial);
  Serial.println("[Sensor] Backend Modbus | RX18 TX17 | Slave 1");
#endif
}

void sensor_update(void) {
  if (millis() - lastRead < kReadInterval) return;
  lastRead = millis();

#if defined(SENSOR_BACKEND_DHT11)
  const float humidity = dht.readHumidity();
  const float temperature = dht.readTemperature();
  if (isnan(temperature) || isnan(humidity)) {
    reading.valid = false;
    Serial.println("[Sensor] DHT11 gagal dibaca");
    return;
  }
  reading = {temperature, humidity, true, millis()};
#else
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
#endif

  Serial.printf("[Sensor] Suhu: %.2f C | Kelembapan: %.2f %%RH\n",
                reading.temperature, reading.humidity);
}

SensorReading sensor_reading(void) { return reading; }