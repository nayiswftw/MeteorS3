#include "src/hal/Backlight.h"
#include "src/Config.h"
#include "src/core/State.h"
#include <esp_arduino_version.h>

namespace hal {

constexpr uint8_t  LEDC_CHANNEL    = 0;
constexpr uint32_t LEDC_FREQ_HZ    = 5000;
constexpr uint8_t  LEDC_RESOLUTION = 8;     // 8-bit: 0 - 255

static uint8_t  s_currentBrightness = 0;
static uint8_t  s_targetBrightness  = 220;
static uint8_t  s_userBrightness    = 220;
static bool     s_isAwake           = true;
static uint32_t s_lastActivityMs    = 0;
static uint32_t s_lastFadeStepMs    = 0;

static void writePwm(uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWrite(config::LCD_BL, duty);
#else
    ledcWrite(LEDC_CHANNEL, duty);
#endif
}

void backlightInit() {
    uint8_t initialBrightness = state::config().backlightBrightness > 0 
                                ? state::config().backlightBrightness 
                                : 220;
    s_userBrightness   = initialBrightness;
    s_targetBrightness = initialBrightness;
    s_currentBrightness= 0;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcAttach(config::LCD_BL, LEDC_FREQ_HZ, LEDC_RESOLUTION);
#else
    ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, LEDC_RESOLUTION);
    ledcAttachPin(config::LCD_BL, LEDC_CHANNEL);
#endif

    writePwm(0);
    s_lastActivityMs = millis();
    s_isAwake = true;
    backlightSetBrightness(initialBrightness, true);
    Serial.printf("[backlight] initialized PWM on GPIO %d (target=%d)\n", config::LCD_BL, initialBrightness);
}

void backlightService() {
    uint32_t now = millis();

    // 1. Smooth Fade Step (every 10ms)
    if (now - s_lastFadeStepMs >= 10) {
        s_lastFadeStepMs = now;
        if (s_currentBrightness != s_targetBrightness) {
            int diff = (int)s_targetBrightness - (int)s_currentBrightness;
            int step = (abs(diff) > 20) ? 8 : (abs(diff) > 5 ? 4 : 1);
            if (diff > 0) {
                s_currentBrightness += step;
                if (s_currentBrightness > s_targetBrightness) s_currentBrightness = s_targetBrightness;
            } else {
                s_currentBrightness -= step;
                if (s_currentBrightness < s_targetBrightness) s_currentBrightness = s_targetBrightness;
            }
            writePwm(s_currentBrightness);
        }
    }

    // 2. Inactivity Timeout (Auto-Sleep / Dim)
    uint32_t timeoutSec = state::config().screenTimeoutSec;
    if (s_isAwake && timeoutSec > 0 && (now - s_lastActivityMs >= (timeoutSec * 1000UL))) {
        backlightSleep();
    }
}

void backlightSetBrightness(uint8_t brightness, bool fade) {
    s_userBrightness = brightness;
    if (s_isAwake) {
        s_targetBrightness = brightness;
        if (!fade) {
            s_currentBrightness = brightness;
            writePwm(brightness);
        }
    }
    s_lastActivityMs = millis();
}

uint8_t backlightGetBrightness() {
    return s_userBrightness;
}

void backlightWake() {
    s_lastActivityMs = millis();
    if (!s_isAwake) {
        s_isAwake = true;
        s_targetBrightness = s_userBrightness;
        Serial.println("[backlight] waking display");
    }
}

void backlightSleep() {
    if (s_isAwake) {
        s_isAwake = false;
        s_targetBrightness = 0;
        Serial.println("[backlight] display sleeping (inactivity)");
    }
}

void backlightToggle() {
    if (s_isAwake) {
        backlightSleep();
    } else {
        backlightWake();
    }
}

bool backlightIsAwake() {
    return s_isAwake;
}

void backlightResetInactivity() {
    s_lastActivityMs = millis();
    if (!s_isAwake) {
        backlightWake();
    }
}

}  // namespace hal
