#include "src/hal/Input.h"
#include "src/hal/Backlight.h"
#include "src/Config.h"
#include <Arduino.h>

namespace hal {

static bool     s_prevButton       = HIGH;
static uint32_t s_buttonDownAt     = 0;

static bool     s_shortEvent       = false;
static bool     s_doubleEvent      = false;
static bool     s_longEvent        = false;
static bool     s_longHandled      = false;

constexpr uint32_t DEBOUNCE_MS     = 30;
constexpr uint32_t LONG_PRESS_MS   = 700;

void inputInit() {
    pinMode(config::BUTTON_PIN, INPUT_PULLUP);
}

void inputService() {
    bool current = digitalRead(config::BUTTON_PIN);
    uint32_t now = millis();

    // 1. Falling edge (button pressed down)
    if (s_prevButton == HIGH && current == LOW) {
        s_buttonDownAt = now;
        s_longHandled  = false;
        backlightResetInactivity(); // Wakes display if sleeping
    }

    // 2. Button held down -> Long Press Trigger
    if (current == LOW && !s_longHandled && (now - s_buttonDownAt >= LONG_PRESS_MS)) {
        s_longHandled = true;
        s_longEvent   = true;
    }

    // 3. Rising edge (button released)
    if (s_prevButton == LOW && current == HIGH) {
        uint32_t pressDuration = now - s_buttonDownAt;
        if (!s_longHandled && pressDuration >= DEBOUNCE_MS) {
            s_shortEvent = true; // Trigger immediately on release
        }
    }

    s_prevButton = current;
}

bool buttonShortPressed() {
    if (!s_shortEvent) return false;
    s_shortEvent = false;
    return true;
}

bool buttonDoublePressed() {
    if (!s_doubleEvent) return false;
    s_doubleEvent = false;
    return true;
}

bool buttonLongPressed() {
    if (!s_longEvent) return false;
    s_longEvent = false;
    return true;
}

bool isButtonPressed() {
    return (digitalRead(config::BUTTON_PIN) == LOW);
}

}  // namespace hal

