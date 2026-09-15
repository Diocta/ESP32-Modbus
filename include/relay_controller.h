#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>

constexpr uint8_t kRelayChannelMin = 1;
constexpr uint8_t kRelayChannelMax = 6;

void relay_init(void);
bool relay_set(bool enabled);
bool relay_is_enabled(void);

// Channel relay yang mengontrol pompa (1-3). Pindah channel butuh wiring manual di device.
bool relay_set_channel(uint8_t channel);
uint8_t relay_get_channel(void);

#endif