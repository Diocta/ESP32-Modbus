#include "mqtt_manager.h"

#include <PubSubClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "relay_controller.h"
#include "sensor_reader.h"

namespace {
WiFiClient plainClients[2];
WiFiClientSecure secureClients[2];
PubSubClient plainMqtt[2] = {PubSubClient(plainClients[0]), PubSubClient(plainClients[1])};
PubSubClient secureMqtt[2] = {PubSubClient(secureClients[0]), PubSubClient(secureClients[1])};
PubSubClient* activeMqtt = nullptr;
MqttProfile profiles[kMaxMqttProfiles];
bool profileUsed[kMaxMqttProfiles] = {};
int activeIndex = -1;
int activeSlot = -1;
String lastError;
unsigned long lastAttempt = 0;
unsigned long lastPublish = 0;

String key(uint8_t index, const char* field) {
  return String("p") + String(index) + "_" + field;
}

void closeMqtt(void) {
  if (activeMqtt) activeMqtt->disconnect();
  activeMqtt = nullptr;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int index = 0; index < length; ++index) message += char(payload[index]);
  message.toLowerCase();
  if (message.indexOf("\"state\":\"on\"") >= 0 || message == "on") {
    relay_set(true);
  } else if (message.indexOf("\"state\":\"off\"") >= 0 || message == "off") {
    relay_set(false);
  }
  (void)topic;
}

void loadProfiles(void) {
  Preferences preferences;
  preferences.begin("aqualab", true);
  activeIndex = preferences.getInt("mqtt_active", -1);
  for (uint8_t index = 0; index < kMaxMqttProfiles; ++index) {
    profiles[index].name = preferences.getString(key(index, "name").c_str(), "");
    profiles[index].broker = preferences.getString(key(index, "host").c_str(), "");
    profiles[index].port = preferences.getUShort(key(index, "port").c_str(), 1883);
    profiles[index].tls = preferences.getBool(key(index, "tls").c_str(), false);
    profiles[index].username = preferences.getString(key(index, "user").c_str(), "");
    profiles[index].password = preferences.getString(key(index, "pass").c_str(), "");
    profileUsed[index] = !profiles[index].name.isEmpty() && !profiles[index].broker.isEmpty();
  }
  preferences.end();
}

bool connectProfile(uint8_t index) {
  if (index >= kMaxMqttProfiles || !profileUsed[index]) return false;
  MqttProfile& profile = profiles[index];
  const int candidateSlot = activeMqtt && activeIndex >= 0 && profiles[activeIndex].tls == profile.tls
                                ? (activeSlot == 0 ? 1 : 0)
                                : 0;
  PubSubClient* candidate = profile.tls ? &secureMqtt[candidateSlot] : &plainMqtt[candidateSlot];
  candidate->setServer(profile.broker.c_str(), profile.port);
  candidate->setCallback(mqttCallback);
  candidate->setBufferSize(512);
  if (profile.tls) secureClients[candidateSlot].setInsecure();
  const String clientId = String("AquaCtrl-") + String((uint32_t)ESP.getEfuseMac(), HEX);
  bool connected = profile.username.isEmpty()
                       ? candidate->connect(clientId.c_str())
                       : candidate->connect(clientId.c_str(), profile.username.c_str(), profile.password.c_str());
  if (!connected) {
    lastError = String("Gagal terhubung (kode MQTT ") + String(candidate->state()) + ")";
    return false;
  }
  if (!candidate->subscribe("pompa/control")) {
    candidate->disconnect();
    lastError = "Profil tersambung, tetapi subscribe pompa/control gagal";
    return false;
  }
  activeMqtt = candidate;
  activeIndex = index;
  activeSlot = candidateSlot;
  lastError = "";
  Serial.printf("[MQTT] Terhubung ke profil: %s\n", profile.name.c_str());
  return true;
}

void publishData(void) {
  if (!activeMqtt || !activeMqtt->connected()) return;
  const SensorReading sensor = sensor_reading();
  String payload = "{\"temperature\":" +
                  (sensor.valid ? String(sensor.temperature, 2) : "null") +
                  ",\"humidity\":" +
                  (sensor.valid ? String(sensor.humidity, 2) : "null") +
                  ",\"pump\":\"" + String(relay_is_enabled() ? "on" : "off") + "\"}";
  activeMqtt->publish("pompa/status", payload.c_str(), true);
  if (sensor.valid) {
    activeMqtt->publish("pompa/sensor/suhu", String(sensor.temperature, 2).c_str(), true);
    activeMqtt->publish("pompa/sensor/kelembapan", String(sensor.humidity, 2).c_str(), true);
  }
}
}  // namespace

void mqtt_init(void) { loadProfiles(); }

void mqtt_update(void) {
  if (activeMqtt && activeMqtt->connected()) {
    activeMqtt->loop();
    if (millis() - lastPublish >= 10000) {
      lastPublish = millis();
      publishData();
    }
    return;
  }
  if (activeIndex >= 0 && millis() - lastAttempt >= 10000) {
    lastAttempt = millis();
    connectProfile(activeIndex);
  }
}

uint8_t mqtt_profile_count(void) {
  uint8_t count = 0;
  for (bool used : profileUsed) count += used ? 1 : 0;
  return count;
}

bool mqtt_get_profile(uint8_t index, MqttProfile& profile) {
  if (index >= kMaxMqttProfiles || !profileUsed[index]) return false;
  profile = profiles[index];
  return true;
}

bool mqtt_save_profile(uint8_t index, const MqttProfile& profile) {
  if (index >= kMaxMqttProfiles || profile.name.isEmpty() || profile.broker.isEmpty()) return false;
  profiles[index] = profile;
  if (profiles[index].port == 0) profiles[index].port = profiles[index].tls ? 8883 : 1883;
  profileUsed[index] = true;
  Preferences preferences;
  preferences.begin("aqualab", false);
  preferences.putString(key(index, "name").c_str(), profiles[index].name);
  preferences.putString(key(index, "host").c_str(), profiles[index].broker);
  preferences.putUShort(key(index, "port").c_str(), profiles[index].port);
  preferences.putBool(key(index, "tls").c_str(), profiles[index].tls);
  preferences.putString(key(index, "user").c_str(), profiles[index].username);
  preferences.putString(key(index, "pass").c_str(), profiles[index].password);
  preferences.end();
  return true;
}

bool mqtt_delete_profile(uint8_t index) {
  if (index >= kMaxMqttProfiles || !profileUsed[index]) return false;
  if (activeIndex == index) closeMqtt();
  Preferences preferences;
  preferences.begin("aqualab", false);
  preferences.remove(key(index, "name").c_str());
  preferences.remove(key(index, "host").c_str());
  preferences.remove(key(index, "port").c_str());
  preferences.remove(key(index, "tls").c_str());
  preferences.remove(key(index, "user").c_str());
  preferences.remove(key(index, "pass").c_str());
  preferences.end();
  profiles[index] = {};
  profileUsed[index] = false;
  if (activeIndex == index) activeIndex = -1;
  if (activeIndex < 0) activeSlot = -1;
  return true;
}

bool mqtt_activate_profile(uint8_t index) {
  if (index >= kMaxMqttProfiles || !profileUsed[index]) return false;
  PubSubClient* previous = activeMqtt;
  if (!connectProfile(index)) return false;
  if (previous && previous != activeMqtt) previous->disconnect();
  Preferences preferences;
  preferences.begin("aqualab", false);
  preferences.putInt("mqtt_active", index);
  preferences.end();
  return true;
}

bool mqtt_is_connected(void) { return activeMqtt && activeMqtt->connected(); }
String mqtt_active_profile_name(void) {
  return activeIndex >= 0 && profileUsed[activeIndex] ? profiles[activeIndex].name : "";
}
String mqtt_last_error(void) { return lastError; }

String mqtt_profiles_json(void) {
  String json = "[";
  bool first = true;
  for (uint8_t index = 0; index < kMaxMqttProfiles; ++index) {
    if (!profileUsed[index]) continue;
    if (!first) json += ",";
    first = false;
    json += "{\"id\":" + String(index) + ",\"name\":\"" + profiles[index].name +
            "\",\"broker\":\"" + profiles[index].broker +
            "\",\"port\":" + String(profiles[index].port) +
            ",\"tls\":" + String(profiles[index].tls ? "true" : "false") +
             ",\"active\":" + String(activeIndex == index ? "true" : "false") + "}";
  }
  json += "]";
  return json;
}

int mqtt_free_profile_index(void) {
  for (uint8_t index = 0; index < kMaxMqttProfiles; ++index) {
    if (!profileUsed[index]) return index;
  }
  return -1;
}