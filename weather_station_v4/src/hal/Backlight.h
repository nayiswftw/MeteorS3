#pragma once

#include <Arduino.h>

namespace hal {

void     backlightInit();
void     backlightService();
void     backlightSetBrightness(uint8_t brightness, bool fade = true);
uint8_t  backlightGetBrightness();
void     backlightWake();
void     backlightSleep();
void     backlightToggle();
bool     backlightIsAwake();
void     backlightResetInactivity();

}  // namespace hal
