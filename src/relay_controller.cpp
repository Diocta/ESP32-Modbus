#include "relay_controller.h"

namespace {
constexpr uint8_t kRelayPin = 1;
bool enabled = false;
}

void relay_init(void) {
  enabled = false;
  digitalWrite(kRelayPin, LOW);
  pinMode(kRelayPin, OUTPUT);
  digitalWrite(kRelayPin, LOW);
  Serial.println("[Relay] Channel 1 | GPIO1 | Active HIGH | OFF saat boot");
}

bool relay_set(bool requested) {
  enabled = requested;
  digitalWrite(kRelayPin, enabled ? HIGH : LOW);
  Serial.printf("[Relay] Pompa %s\n", enabled ? "ON" : "OFF");
  return enabled;
}

bool relay_is_enabled(void) { return enabled; }