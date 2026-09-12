#include "wifi_config.h"
#include <DNSServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WebServer.h>

namespace {
constexpr char kConfigSsid[] = "ESP32-S3";
constexpr char kMdnsName[] = "aqua";
constexpr char kConfigRequestKey[] = "cfg_req";
constexpr byte kDnsPort = 53;

WebServer webServer(80);
DNSServer dnsServer;
bool configPortalActive = false;
bool webServerStarted = false;
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

void sendIndex() {
  if (!serveFile("/index.html")) {
    webServer.send(500, "text/plain", "index.html tidak tersedia");
  }
}

void sendStatus() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  String json = "{\"connected\":" + String(connected ? "true" : "false") +
                ",\"configMode\":" +
                String(configPortalActive ? "true" : "false") +
                ",\"ssid\":\"" +
                (connected ? WiFi.SSID() : String(kConfigSsid)) +
                "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  webServer.send(200, "application/json", json);
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

void startConfigPortal() {
  configPortalActive = true;
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(kMdnsName);
  bool apStarted = WiFi.softAP(kConfigSsid);
  if (!apStarted) {
    delay(100);
    apStarted = WiFi.softAP(kConfigSsid);
  }
  Serial.printf("[WiFi] AP %s: %s\n", kConfigSsid, apStarted ? "aktif" : "gagal");
  Serial.printf("[WiFi] IP konfigurasi: %s\n", WiFi.softAPIP().toString().c_str());
  dnsServer.start(kDnsPort, "*", WiFi.softAPIP());
  startWebServer();
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
    startWebServer();
    return;
  }

  startConfigPortal();
}

void wifi_handle_client(void) {
  if (configPortalActive) dnsServer.processNextRequest();
  if (webServerStarted) webServer.handleClient();
}

void wifi_set_mode(uint8_t mode) {
  WiFi.mode(mode == WIFI_MODE_CONFIG ? WIFI_AP_STA : WIFI_STA);
}

bool wifi_is_connected(void) { return WiFi.status() == WL_CONNECTED; }
