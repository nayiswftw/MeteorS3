/*
 * ================================================================
 *  WEATHER STATION V4
 *  Waveshare ESP32-S3-LCD-2
 * ================================================================
 *
 *  PlatformIO firmware project.
 *  Clean layered architecture.
 *  Zero-allocation data paths & pure UI components.
 */

#include <Arduino.h>

#include "Config.h"
#include "core/State.h"
#include "hal/Display.h"
#include "hal/Input.h"
#include "hal/Battery.h"
#include "hal/Imu.h"
#include "services/Network.h"
#include "services/TimeSync.h"
#include "services/Api.h"
#include "services/Storage.h"
#include "ui/Screen.h"

static uint32_t s_lastClockMinute = 0;
static uint32_t s_lastPageChange  = 0;

void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("================================");
    Serial.println(" WEATHER STATION V4 (PlatformIO)");
    Serial.println(" Waveshare ESP32-S3-LCD-2");
    Serial.println("================================");

    // 1. Hardware Abstraction Layer
    hal::inputInit();
    hal::batteryInit();
    hal::imuInit();
    hal::displayInit();

    // 2. Storage & Cache (restore offline conditions before networking)
    svc::storageInit();

    // 3. UI Initialization (paint first screen immediately from cache)
    screen::init();

    // 4. Services (NTP, WiFi, API)
    svc::timeSyncInit();
    svc::networkInit();
    svc::apiInit();

    s_lastPageChange = millis();
    Serial.println("[boot] ready");
}

void loop() {
    // 1. Drive Display & Input
    hal::displayService();
    hal::inputService();

    // 2. Handle User Button Actions
    if (hal::buttonLongPressed()) {
        screen::home();
        s_lastPageChange = millis();
    }

    if (hal::buttonShortPressed()) {
        screen::next();
        s_lastPageChange = millis();
    }

    // 3. Track state snapshot for change detection
    bool   beforeWifi    = state::isOnline();
    time_t beforeWeather = state::weather().fetchedAt;
    time_t beforeAir     = state::air().fetchedAt;

    // 4. Run Services
    svc::networkService();
    svc::apiService();
    svc::storageService();

    // 5. Repaint when connectivity or data updates
    if (beforeWifi != state::isOnline() ||
        beforeWeather != state::weather().fetchedAt ||
        beforeAir != state::air().fetchedAt) {
        screen::refresh();
    }

    // 6. Refresh clock once per minute
    uint32_t currentMinute = millis() / 60000UL;
    if (currentMinute != s_lastClockMinute) {
        s_lastClockMinute = currentMinute;
        screen::refresh();
    }

    // 7. Auto page rotation (if enabled in Config.h)
    if (config::ENABLE_AUTO_PAGE &&
        (millis() - s_lastPageChange >= config::AUTO_PAGE_MS)) {
        screen::next();
        s_lastPageChange = millis();
    }

    delay(5);
}
