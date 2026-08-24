#include "src/hal/Input.h"
#include "src/Config.h"
#include <Arduino.h>

namespace hal {

static bool     s_prevButton   = HIGH;
static uint32_t s_buttonDownAt = 0;
static bool     s_shortEvent   = false;
static bool     s_longEvent    = false;
static bool     s_longHandled  = false;

constexpr uint32_t DEBOUNCE_MS   = 30;
constexpr uint32_t LONG_PRESS_MS = 900;

void inputInit() {
    pinMode(config::BUTTON_PIN, INPUT_PULLUP);
}

void inputService() {
    bool current = digitalRead(config::BUTTON_PIN);

    // Falling edge (press)
    if (s_prevButton == HIGH && current == LOW) {
        s_buttonDownAt = millis();
        s_longHandled  = false;
    }

    // Holding (check for long press threshold)
    if (current == LOW && !s_longHandled && (millis() - s_buttonDownAt >= LONG_PRESS_MS)) {
        s_longHandled = true;
        s_longEvent   = true;
    }

    // Rising edge (release)
    if (s_prevButton == LOW && current == HIGH) {
        uint32_t duration = millis() - s_buttonDownAt;
        if (!s_longHandled && duration >= DEBOUNCE_MS) {
            s_shortEvent = true;
        }
    }

    s_prevButton = current;
}

bool buttonShortPressed() {
    if (!s_shortEvent) return false;
    s_shortEvent = false;
    return true;
}

bool buttonLongPressed() {
    if (!s_longEvent) return false;
    s_longEvent = false;
    return true;
}

}  // namespace hal
