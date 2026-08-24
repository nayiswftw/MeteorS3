/*
 * ================================================================
 *  WEATHER STATION V4
 *  Waveshare ESP32-S3-LCD-2
 * ================================================================
 *
 *  Arduino IDE Firmware Project
 *  Clean layered architecture.
 *  Zero-allocation data paths & pure UI components.
 *
 *  Required Board Settings in Arduino IDE (Tools menu):
 *  ----------------------------------------------------
 *  - Board: "ESP32S3 Dev Module"
 *  - Flash Size: "16MB (128Mb)"
 *  - Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)" or "Default 16MB with spiffs"
 *  - PSRAM: "OPI PSRAM"
 *  - USB CDC On Boot: "Enabled"
 *  - Upload Speed: "921600"
 *
 *  Required Libraries (install via Arduino IDE Library Manager):
 *  -------------------------------------------------------------
 *  - ArduinoJson (v7.x)
 *  - GFX Library for Arduino (v1.6.x+)
 *  - lvgl (v9.5.x+)
 */

#include <Arduino.h>

#include "src/Config.h"
#include "src/core/State.h"
#include "src/hal/Display.h"
#include "src/hal/Input.h"
#include "src/hal/Battery.h"
#include "src/hal/Imu.h"
#include "src/services/Network.h"
#include "src/services/TimeSync.h"
#include "src/services/Api.h"
#include "src/services/Storage.h"
#include "src/ui/Screen.h"

static uint32_t s_lastClockMinute = 0;
static uint32_t s_lastPageChange  = 0;

void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("================================");
    Serial.println(" WEATHER STATION V4 (Arduino IDE)");
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
