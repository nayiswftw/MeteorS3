#include "src/hal/Battery.h"
#include "src/Config.h"
#include <Arduino.h>

namespace hal {

void batteryInit() {
    analogReadResolution(12);
}

float readBatteryVoltage() {
    uint32_t total = 0;
    constexpr int SAMPLES = 8;

    for (int i = 0; i < SAMPLES; i++) {
        total += analogRead(config::BATTERY_ADC);
        delayMicroseconds(80);
    }

    float raw = (float)total / (float)SAMPLES;

    // Voltage divider (2:1 ratio), 3.3V reference, 12-bit ADC (4095)
    return (raw * 3.3f / 4095.0f) * 2.0f;
}

int readBatteryPercent() {
    float v = readBatteryVoltage();

    // If no battery is connected / running on USB power (ADC reads floating or near zero)
    if (v < 2.80f || v > 4.40f) {
        return -1; // -1 denotes USB / Direct Power
    }

    if (v >= 4.20f) return 100;
    if (v >= 4.00f) return map((int)(v * 1000), 4000, 4200, 80, 100);
    if (v >= 3.80f) return map((int)(v * 1000), 3800, 4000, 45, 80);
    if (v >= 3.65f) return map((int)(v * 1000), 3650, 3800, 15, 45);
    if (v >= 3.40f) return map((int)(v * 1000), 3400, 3650, 0, 15);

    return 0;
}

}  // namespace hal
