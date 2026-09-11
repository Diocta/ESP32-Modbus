#include <Arduino.h>
#include <ModbusMaster.h>

// Pin RS485 Onboard Waveshare ESP32-S3 Relay 6CH
#define RX_PIN 18
#define TX_PIN 17
#define SLAVE_ID 1

HardwareSerial modbusSerial(2);
ModbusMaster node;

void setup() {
  Serial.begin(115200);
  delay(2000); // Waktu inisialisasi power sensor Autonics (min 2 detik)

  // Auto-wakeup USB CDC Waveshare ESP32-S3
  for (int i = 0; i < 20; i++) {
    Serial.print((char)random(33, 126));
    delay(50);
  }
  Serial.println("\n[OK] USB Serial Aktif!");
  Serial.println("=================================================");
  Serial.println("   PEMBACAAN SENSOR AUTONICS THD (ESP32-S3)     ");
  Serial.println("=================================================");

  // Inisialisasi UART RS485 pada GPIO 43 & 44 (9600 8N1)
  modbusSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  node.begin(SLAVE_ID, modbusSerial);
}

void loop() {
  // Read 2 Input Registers dari Address 0x0000 (300001 = Suhu, 300002 = Kelembapan)
  uint8_t result = node.readInputRegisters(0x0000, 2);

  if (result == node.ku8MBSuccess) {
    uint16_t rawTemp = node.getResponseBuffer(0); // Register 0x0000
    uint16_t rawHumi = node.getResponseBuffer(1); // Register 0x0001

    // Konversi nilai integer ke float (dibagi 100.0 sesuai datasheet Autonics)
    float temperature = rawTemp / 100.0;
    float humidity    = rawHumi / 100.0;

    // Penanganan suhu negatif (signed int)
    if (rawTemp > 0x7FFF) {
      temperature = (rawTemp - 0x10000) / 100.0;
    }

    Serial.println("-------------------------------------------------");
    Serial.printf("Suhu       : %.2f °C\n", temperature);
    Serial.printf("Kelembapan : %.2f %%RH\n", humidity);
    Serial.println("-------------------------------------------------");
  } else {
    Serial.printf("[!] Gagal Membaca Sensor! Error: 0x%02X\n", result);
  }

  delay(2000);
}