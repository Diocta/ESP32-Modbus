#include <Arduino.h>
#include "relay_controller.h"
#include "mqtt_manager.h"
#include "sensor_reader.h"
#include "wifi_config.h"

void setup() {
  Serial.begin(115200);
  delay(2000); // Waktu inisialisasi power sensor Autonics (min 2 detik)

  // INISIALISASI WIFI SETELAH SERIAL AGAR LOG WIFI TERLIHAT
  wifi_init();

  sensor_init();
  relay_init();
  mqtt_init();
}

void loop() {
  wifi_handle_client();

  // WiFi.setAutoReconnect(true) menangani reconnect di background;
  // tidak perlu panggil wifi_init() ulang (blocking) di sini.

  sensor_update();
  mqtt_update();
  delay(10);
}