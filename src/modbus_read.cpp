#include <Arduino.h>
#include "relay_controller.h"
#include "sensor_reader.h"
#include "wifi_config.h"

void setup() {
  Serial.begin(115200);
  delay(2000); // Waktu inisialisasi power sensor Autonics (min 2 detik)

  // INISIALISASI WIFI SETELAH SERIAL AGAR LOG WIFI TERLIHAT
  wifi_init();

  sensor_init();
  relay_init();
}

void loop() {
  wifi_handle_client();

  // Pastikan WiFi tetap connected
  if (WiFi.status() != WL_CONNECTED) {
    wifi_init();
  }

  sensor_update();
  delay(10);
}