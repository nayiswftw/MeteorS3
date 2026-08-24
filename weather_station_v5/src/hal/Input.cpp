#include "src/hal/Input.h"
#include "src/hal/Backlight.h"
#include "src/Config.h"
#include <Arduino.h>

namespace hal {

static bool     s_prevButton       = HIGH;
static uint32_t s_buttonDownAt     = 0;
static uint32_t s_lastReleaseAt    = 0;
static int      s_clickCount       = 0;

static bool     s_shortEvent       = false;
static bool     s_doubleEvent      = false;
static bool     s_longEvent        = false;
static bool     s_longHandled      = false;

constexpr uint32_t DEBOUNCE_MS     = 25;
constexpr uint32_t DOUBLE_CLICK_MS = 280;
constexpr uint32_t LONG_PRESS_MS   = 750;

void inputInit() {
    pinMode(config::BUTTON_PIN, INPUT_PULLUP);
}

void inputService() {
    bool current = digitalRead(config::BUTTON_PIN);
    uint32_t now = millis();

    // 1. Falling edge (button pressed)
    if (s_prevButton == HIGH && current == LOW) {
        s_buttonDownAt = now;
        s_longHandled  = false;
        backlightResetInactivity(); // button press wakes backlight
    }

    // 2. Button held down -> Long Press Trigger
    if (current == LOW && !s_longHandled && (now - s_buttonDownAt >= LONG_PRESS_MS)) {
        s_longHandled = true;
        s_longEvent   = true;
        s_clickCount  = 0; // abort double click
    }

    // 3. Rising edge (button released)
    if (s_prevButton == LOW && current == HIGH) {
        uint32_t pressDuration = now - s_buttonDownAt;
        if (!s_longHandled && pressDuration >= DEBOUNCE_MS) {
            s_clickCount++;
            s_lastReleaseAt = now;
        }
    }

    // 4. Double click timeout evaluation
    if (s_clickCount > 0 && (now - s_lastReleaseAt > DOUBLE_CLICK_MS)) {
        if (s_clickCount == 1) {
            s_shortEvent = true;
        } else if (s_clickCount >= 2) {
            s_doubleEvent = true;
        }
        s_clickCount = 0;
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

