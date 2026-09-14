#include "wifi_config.h"
#include "relay_controller.h"
#include "sensor_reader.h"
#include "mqtt_manager.h"
#include <DNSServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WebServer.h>

namespace {
constexpr char kConfigSsid[] = "ESP32-S3";
constexpr char kConfigPassword[] = "esp32pompa";
constexpr char kMdnsName[] = "aqua";
constexpr char kConfigRequestKey[] = "cfg_req";
constexpr byte kDnsPort = 53;

WebServer webServer(80);
DNSServer dnsServer;
bool configPortalActive = false;
bool accessPointActive = false;
bool webServerStarted = false;
unsigned long lastWifiLog = 0;
char saved_ssid[33] = "";
char saved_password[65] = "";

String contentType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
}

bool serveFile(const String& path) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  webServer.streamFile(file, contentType(path));
  file.close();
  return true;
}

String jsonEscape(const String& value) {
  String escaped = value;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");
  return escaped;
}

void sendIndex() {
  if (!serveFile("/index.html")) {
    webServer.send(500, "text/plain", "index.html tidak tersedia");
  }
}

void sendStatus() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  const SensorReading sensor = sensor_reading();
  String json = "{\"connected\":" + String(connected ? "true" : "false") +
                ",\"configMode\":" +
                String(configPortalActive ? "true" : "false") +
                ",\"ssid\":\"" +
                (connected ? WiFi.SSID() : String(kConfigSsid)) +
                "\",\"ip\":\"" + WiFi.localIP().toString() +
                "\",\"temperature\":" +
                (sensor.valid ? String(sensor.temperature, 2) : "null") +
                ",\"humidity\":" +
                (sensor.valid ? String(sensor.humidity, 2) : "null") +
                ",\"sensorValid\":" +
                String(sensor.valid ? "true" : "false") +
                ",\"pump\":" +
                String(relay_is_enabled() ? "true" : "false") +
                ",\"mqttConnected\":" +
                String(mqtt_is_connected() ? "true" : "false") +
                ",\"mqttProfile\":\"" + jsonEscape(mqtt_active_profile_name()) +
                  "\",\"mqttError\":\"" + jsonEscape(mqtt_last_error()) +
                  "\",\"deviceId\":" + String(sensor_get_device_id()) +
                  ",\"relayChannel\":" + String(relay_get_channel()) + "}";
  webServer.send(200, "application/json", json);
}

void setPump() {
  const String state = webServer.arg("state");
  if (state != "on" && state != "off") {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"state harus on atau off\"}");
    return;
  }
  const bool enabled = relay_set(state == "on");
  webServer.send(200, "application/json",
                 String("{\"ok\":true,\"pump\":") +
                     (enabled ? "true}" : "false}"));
}

void sendMqttProfiles() {
  webServer.send(200, "application/json", mqtt_profiles_json());
}

void saveMqttProfile() {
  int index = webServer.arg("id").toInt();
  if (index < 0) index = mqtt_free_profile_index();
  MqttProfile profile;
  profile.name = webServer.arg("name");
  profile.broker = webServer.arg("broker");
  profile.port = webServer.arg("port").toInt();
  profile.tls = webServer.arg("tls") == "true";
  profile.username = webServer.arg("username");
  profile.password = webServer.arg("password");
  if (index < 0 || index >= kMaxMqttProfiles || profile.name.isEmpty() ||
      profile.broker.isEmpty() || profile.port == 0 ||
      !mqtt_save_profile(index, profile)) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"Data profil tidak valid\"}");
    return;
  }
  webServer.send(200, "application/json", "{\"ok\":true}");
}

void activateMqttProfile() {
  const int index = webServer.arg("id").toInt();
  if (index < 0 || index >= kMaxMqttProfiles || !mqtt_activate_profile(index)) {
    webServer.send(200, "application/json",
                   String("{\"ok\":false,\"error\":\"") + jsonEscape(mqtt_last_error()) + "\"}");
    return;
  }
  webServer.send(200, "application/json", "{\"ok\":true}");
}

void deleteMqttProfile() {
  const int index = webServer.arg("id").toInt();
  const bool deleted = index >= 0 && index < kMaxMqttProfiles && mqtt_delete_profile(index);
  webServer.send(200, "application/json", String("{\"ok\":") + (deleted ? "true}" : "false}"));
}

void setDeviceId() {
  const int id = webServer.arg("id").toInt();
  if (id < kSensorDeviceIdMin || id > kSensorDeviceIdMax || !sensor_set_device_id((uint8_t)id)) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"Device ID harus 1-3\"}");
    return;
  }
  webServer.send(200, "application/json", "{\"ok\":true}");
}

void setRelayChannel() {
  const int channel = webServer.arg("channel").toInt();
  if (channel < kRelayChannelMin || channel > kRelayChannelMax || !relay_set_channel((uint8_t)channel)) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"Channel relay harus 1-3\"}");
    return;
  }
  webServer.send(200, "application/json", "{\"ok\":true}");
}

void sendScan() {
  const int count = WiFi.scanNetworks(false, true);
  String json = "[";
  for (int index = 0; index < count; ++index) {
    if (index > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(index) + "\",\"rssi\":" +
            String(WiFi.RSSI(index)) + "}";
  }
  json += "]";
  WiFi.scanDelete();
  webServer.send(200, "application/json", json);
}

void saveCredentials(const String& ssid, const String& password) {
  Preferences preferences;
  preferences.begin("aqualab", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", password);
  preferences.end();
}

void requestConfigMode() {
  Preferences preferences;
  preferences.begin("aqualab", false);
  const size_t written = preferences.putBool(kConfigRequestKey, true);
  preferences.end();
  if (written == 0) {
    Serial.println("[WiFi] Gagal menyimpan permintaan mode konfigurasi");
    webServer.send(500, "application/json", "{\"ok\":false,\"error\":\"Mode konfigurasi gagal disimpan\"}");
    return;
  }
  Serial.println("[WiFi] Mode konfigurasi diminta, ESP32 akan restart");
  webServer.send(200, "application/json", "{\"ok\":true,\"restarting\":true}");
  delay(500);
  ESP.restart();
}

void connectToNewWifi() {
  const String ssid = webServer.arg("ssid");
  const String password = webServer.arg("password");
  if (ssid.isEmpty()) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"SSID wajib diisi\"}");
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(false);
  WiFi.begin(ssid.c_str(), password.c_str());
  const unsigned long deadline = millis() + 20000;
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) delay(100);

  if (WiFi.status() != WL_CONNECTED) {
    webServer.send(200, "application/json", "{\"ok\":false,\"error\":\"Tidak dapat terhubung ke WiFi\"}");
    return;
  }

  saveCredentials(ssid, password);
  strncpy(saved_ssid, ssid.c_str(), sizeof(saved_ssid) - 1);
  saved_ssid[sizeof(saved_ssid) - 1] = '\0';
  strncpy(saved_password, password.c_str(), sizeof(saved_password) - 1);
  saved_password[sizeof(saved_password) - 1] = '\0';
  webServer.send(200, "application/json", "{\"ok\":true,\"restarting\":true}");
  delay(800);
  ESP.restart();
}

void registerRoutes() {
  webServer.on("/", HTTP_GET, sendIndex);
  webServer.on("/index.html", HTTP_GET, sendIndex);
  webServer.on("/style.css", HTTP_GET, []() { serveFile("/style.css"); });
  webServer.on("/script.js", HTTP_GET, []() { serveFile("/script.js"); });
  webServer.on("/api/status", HTTP_GET, sendStatus);
  webServer.on("/api/pump", HTTP_POST, setPump);
  webServer.on("/api/mqtt/profiles", HTTP_GET, sendMqttProfiles);
  webServer.on("/api/mqtt/profile", HTTP_POST, saveMqttProfile);
  webServer.on("/api/mqtt/activate", HTTP_POST, activateMqttProfile);
  webServer.on("/api/mqtt/delete", HTTP_POST, deleteMqttProfile);
  webServer.on("/api/modbus/device", HTTP_POST, setDeviceId);
  webServer.on("/api/relay/channel", HTTP_POST, setRelayChannel);
  webServer.on("/api/wifi/scan", HTTP_GET, sendScan);
  webServer.on("/api/wifi/connect", HTTP_POST, connectToNewWifi);
  webServer.on("/api/wifi/configure", HTTP_POST, requestConfigMode);
  webServer.on("/generate_204", HTTP_GET, sendIndex);
  webServer.on("/hotspot-detect.html", HTTP_GET, sendIndex);
  webServer.on("/connecttest.txt", HTTP_GET, sendIndex);
  webServer.on("/ncsi.txt", HTTP_GET, sendIndex);
  webServer.on("/fwlink", HTTP_GET, sendIndex);
  webServer.onNotFound([]() {
    if (configPortalActive) sendIndex();
    else webServer.send(404, "text/plain", "Not found");
  });
}

void startWebServer() {
  if (webServerStarted) return;
  if (!LittleFS.begin(true)) return;
  registerRoutes();
  webServer.begin();
  webServerStarted = true;
}

void startAccessPoint(bool disconnectStation) {
  if (disconnectStation) WiFi.disconnect(true, false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(kMdnsName);
  bool apStarted = WiFi.softAP(kConfigSsid, kConfigPassword);
  if (!apStarted) {
    delay(100);
    apStarted = WiFi.softAP(kConfigSsid, kConfigPassword);
  }
  accessPointActive = apStarted;
  Serial.printf("[WiFi] AP %s: %s\n", kConfigSsid, apStarted ? "aktif" : "gagal");
  Serial.printf("[WiFi] IP konfigurasi: %s\n", WiFi.softAPIP().toString().c_str());
  dnsServer.start(kDnsPort, "*", WiFi.softAPIP());
  startWebServer();
}

void startConfigPortal() {
  configPortalActive = true;
  startAccessPoint(true);
}
}  // namespace

const char* wifi_get_saved_ssid(void) { return saved_ssid; }
const char* wifi_get_saved_password(void) { return saved_password; }

void wifi_init(void) {
  if (configPortalActive) return;

  Preferences preferences;
  preferences.begin("aqualab", true);
  const bool configRequested = preferences.getBool(kConfigRequestKey, false);
  const String storedSsid = preferences.getString("ssid", "");
  const String storedPassword = preferences.getString("pass", "");
  preferences.end();

  if (configRequested) {
    preferences.begin("aqualab", false);
    preferences.putBool(kConfigRequestKey, false);
    preferences.end();
    startConfigPortal();
    return;
  }

  if (!storedSsid.isEmpty()) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(storedSsid.c_str(), storedPassword.c_str());
    const unsigned long deadline = millis() + 20000;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    configPortalActive = false;
    strncpy(saved_ssid, WiFi.SSID().c_str(), sizeof(saved_ssid) - 1);
    saved_ssid[sizeof(saved_ssid) - 1] = '\0';
    WiFi.setHostname(kMdnsName);
    WiFi.setAutoReconnect(true);
    Serial.printf("[WiFi] Terhubung ke %s\n", WiFi.SSID().c_str());
    Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    MDNS.begin(kMdnsName);
    startAccessPoint(false);
    startWebServer();
    return;
  }

  startConfigPortal();
}

void wifi_handle_client(void) {
  if (accessPointActive) dnsServer.processNextRequest();
  if (webServerStarted) webServer.handleClient();
  if (millis() - lastWifiLog >= 10000) {
    lastWifiLog = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WiFi] Terhubung ke %s | IP: %s | Dashboard: http://aqua.local\n",
                    WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    } else if (accessPointActive) {
      Serial.printf("[WiFi] Mode konfigurasi | AP: %s | Buka: http://192.168.4.1\n",
                    kConfigSsid);
    } else {
      Serial.println("[WiFi] Belum terhubung");
    }
  }
}

void wifi_set_mode(uint8_t mode) {
  WiFi.mode(mode == WIFI_MODE_CONFIG ? WIFI_AP_STA : WIFI_STA);
}

bool wifi_is_connected(void) { return WiFi.status() == WL_CONNECTED; }
