#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

// Mode operasi WiFi
#define WIFI_MODE_AUTO      0
#define WIFI_MODE_CONFIG    1

// Fungsi inisialisasi WiFi (dijalankan di setup())
void wifi_init(void);

// Set mode operasi (AP config atau auto connect)
void wifi_set_mode(uint8_t mode);

// Cek status koneksi
bool wifi_is_connected(void);

void wifi_handle_client(void);

// Ambil SSID yang tersimpan
const char* wifi_get_saved_ssid(void);

// Ambil password yang tersimpan
const char* wifi_get_saved_password(void);

#endif