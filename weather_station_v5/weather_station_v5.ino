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
#include "src/hal/Backlight.h"
#include "src/hal/Input.h"
#include "src/hal/Battery.h"
#include "src/hal/Imu.h"
#include "src/services/Network.h"
#include "src/services/TimeSync.h"
#include "src/services/Api.h"
#include "src/services/Storage.h"
#include "src/services/WebServer.h"
#include "src/services/Mqtt.h"
#include "src/ui/Screen.h"

static uint32_t s_lastClockMinute = 0;
static uint32_t s_lastPageChange  = 0;

void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("=========================================");
    Serial.println(" METEORS3 — WEATHER STATION V5");
    Serial.println(" Waveshare ESP32-S3-LCD-2");
    Serial.println("=========================================");

    // 0. State Mutex Initialization
    state::init();

    // 1. Hardware Abstraction Layer
    hal::inputInit();
    hal::batteryInit();
    hal::imuInit();
    hal::displayInit();

    // 2. Storage & Runtime Preferences
    svc::storageInit();

    // 3. Check for BOOT Button Held for AP Setup Mode
    if (hal::isButtonPressed()) {
        Serial.println("[boot] BOOT button held — forcing AP Setup Mode!");
        svc::networkStartApMode();
    }

    // 4. UI Initialization (paint first screen immediately from cache)
    screen::init();

    // 5. Services (NTP, WiFi, Async API Worker Core 0, Web Server, MQTT)
    svc::timeSyncInit();
    if (!svc::networkIsApMode()) {
        svc::networkInit();
    }
    svc::apiInit();
    svc::webServerInit();
    svc::mqttInit();

    s_lastPageChange = millis();
    Serial.println("[boot] MeteorS3 fully initialized and ready!");
}

void loop() {
    // 1. Drive Hardware & Display Loop (Core 1)
    hal::displayService();
    hal::inputService();
    hal::imuService();
    hal::backlightService();

    // 2. Drive Network & Web/MQTT Services
    svc::networkService();
    svc::apiService();
    svc::webServerService();
    svc::mqttService();
    svc::storageService();

    // 3. Handle Physical Button Actions
    if (hal::buttonLongPressed()) {
        if (!hal::backlightIsAwake()) {
            hal::backlightWake();
        } else {
            screen::home();
        }
        s_lastPageChange = millis();
    } else if (hal::buttonShortPressed()) {
        if (!hal::backlightIsAwake()) {
            hal::backlightWake();
        } else {
            screen::next();
        }
        s_lastPageChange = millis();
    }

    // 4. Handle IMU 6-Axis Motion Gestures
    GestureType gesture = hal::imuReadGesture();
    if (gesture != GestureType::NONE) {
        if (!hal::backlightIsAwake()) {
            hal::backlightWake();
        } else {
            if (gesture == GestureType::TILT_RIGHT) {
                screen::next();
            } else if (gesture == GestureType::TILT_LEFT) {
                screen::prev();
            } else if (gesture == GestureType::SHAKE) {
                screen::home();
            }
        }
        s_lastPageChange = millis();
    }

    // 5. Track state snapshot for instant responsive repaint
    uint32_t stateRev = state::stateRevision();
    static uint32_t s_lastStateRev = 0;

    if (stateRev != s_lastStateRev) {
        s_lastStateRev = stateRev;
        screen::refresh();
    }

    // 6. Refresh clock once per minute
    uint32_t currentMinute = millis() / 60000UL;
    if (currentMinute != s_lastClockMinute) {
        s_lastClockMinute = currentMinute;
        screen::refresh();
    }

    // 7. Auto page rotation (if enabled and display awake)
    state::lock();
    bool     autoPageEn  = state::config().enableAutoPage;
    uint32_t autoPageSec = state::config().autoPageSec;
    state::unlock();

    if (autoPageEn && hal::backlightIsAwake() && (millis() - s_lastPageChange >= (autoPageSec * 1000UL))) {
        screen::next();
        s_lastPageChange = millis();
    }

    delay(4);
}

