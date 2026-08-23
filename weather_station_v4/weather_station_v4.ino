/*
  ================================================================
   WEATHER STATION V4
   Waveshare ESP32-S3-LCD-2
  ================================================================

   Arduino IDE project.
   Modular firmware.
   Reusable LVGL UI components.
   Broad weather-station feature set.

   Libraries:
     ArduinoJson 7.x
     Arduino_GFX_Library
     LVGL 9.x
*/

#include <Arduino.h>

#include "Config.h"
#include "Hardware.h"
#include "DataService.h"
#include "StorageService.h"
#include "Screens.h"
#include "AppState.h"

static uint32_t lastUiClockMinute = 0;
static uint32_t lastPageChange = 0;

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("================================");
  Serial.println(" WEATHER STATION V4");
  Serial.println(" ESP32-S3-LCD-2");
  Serial.println("================================");

  hardwareBegin();
  displayBegin();

  // Restore current conditions before networking.
  storageBegin();

  // First paint never waits on WiFi.
  screensBegin();

  dataBegin();

  lastPageChange = millis();

  Serial.println("[boot] ready");
}

void loop() {
  displayService();

  if (buttonLongPressed()) {
    screensHome();
    lastPageChange = millis();
  }

  if (buttonShortPressed()) {
    screensNext();
    lastPageChange = millis();
  }

  bool beforeWifi = wifiConnected;
  time_t beforeWeather = weather.fetchedAt;
  time_t beforeAir = air.fetchedAt;

  dataService();
  storageService();

  // Repaint when connectivity/data changes.
  if (beforeWifi != wifiConnected ||
      beforeWeather != weather.fetchedAt ||
      beforeAir != air.fetchedAt) {
    screensShow(screensCurrent());
  }

  // Refresh the clock once per minute.
  uint32_t minute = millis() / 60000UL;

  if (minute != lastUiClockMinute) {
    lastUiClockMinute = minute;
    screensShow(screensCurrent());
  }

  if (ENABLE_AUTO_PAGE &&
      millis() - lastPageChange >= AUTO_PAGE_MS) {
    screensNext();
    lastPageChange = millis();
  }

  delay(5);
}
