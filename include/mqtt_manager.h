#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>

constexpr uint8_t kMaxMqttProfiles = 5;

struct MqttProfile {
  String name;
  String broker;
  uint16_t port;
  bool tls;
  String username;
  String password;
};

void mqtt_init(void);
void mqtt_update(void);
uint8_t mqtt_profile_count(void);
int mqtt_free_profile_index(void);
bool mqtt_get_profile(uint8_t index, MqttProfile& profile);
bool mqtt_save_profile(uint8_t index, const MqttProfile& profile);
bool mqtt_delete_profile(uint8_t index);
bool mqtt_activate_profile(uint8_t index);
bool mqtt_is_connected(void);
String mqtt_active_profile_name(void);
String mqtt_last_error(void);
String mqtt_profiles_json(void);

#endif