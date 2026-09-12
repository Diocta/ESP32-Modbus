#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>

void relay_init(void);
bool relay_set(bool enabled);
bool relay_is_enabled(void);

#endif