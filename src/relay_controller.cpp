#include "relay_controller.h"

#include <Preferences.h>

namespace {
constexpr uint8_t kChannelPins[6] = {1, 2, 41, 42, 45, 46};  // CH1-CH6
uint8_t channel = 1;
bool enabled = false;

uint8_t pinForChannel(uint8_t ch) { return kChannelPins[ch - 1]; }
}

void relay_init(void) {
  Preferences preferences;
  preferences.begin("aqualab", true);
  channel = preferences.getUChar("relay_ch", 1);
  preferences.end();
  if (channel < kRelayChannelMin || channel > kRelayChannelMax) channel = 1;

  enabled = false;
  const uint8_t pin = pinForChannel(channel);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  Serial.printf("[Relay] Channel %d | GPIO%d | Active HIGH | OFF saat boot\n", channel, pin);
}

bool relay_set(bool requested) {
  enabled = requested;
  digitalWrite(pinForChannel(channel), enabled ? HIGH : LOW);
  Serial.printf("[Relay] Pompa %s\n", enabled ? "ON" : "OFF");
  return enabled;
}

bool relay_is_enabled(void) { return enabled; }

bool relay_set_channel(uint8_t newChannel) {
  if (newChannel < kRelayChannelMin || newChannel > kRelayChannelMax) return false;
  if (newChannel != channel) {
    digitalWrite(pinForChannel(channel), LOW);
    pinMode(pinForChannel(channel), INPUT);
    channel = newChannel;
    pinMode(pinForChannel(channel), OUTPUT);
    digitalWrite(pinForChannel(channel), enabled ? HIGH : LOW);
    Preferences preferences;
    preferences.begin("aqualab", false);
    preferences.putUChar("relay_ch", channel);
    preferences.end();
    Serial.printf("[Relay] Channel diganti ke %d (GPIO%d)\n", channel, pinForChannel(channel));
  }
  return true;
}

uint8_t relay_get_channel(void) { return channel; }