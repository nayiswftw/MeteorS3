#pragma once
#include <Arduino.h>

// ============================================================
// WEATHER STATION V4 — CONFIG
// ============================================================

// ---- User --------------------------------------------------------
inline constexpr char WIFI_SSID[]     = "Xiaomi 11i HyperCharge";
inline constexpr char WIFI_PASSWORD[] = "12345678";

inline constexpr double LATITUDE  = 30.000000;
inline constexpr double LONGITUDE = 75.000000;

inline constexpr char LOCATION_NAME[] = "HOME";
inline constexpr char TIMEZONE[]      = "Asia/Kolkata";

// ---- Units -------------------------------------------------------
enum TemperatureUnit { TEMP_C, TEMP_F };
enum WindUnit { WIND_KMH, WIND_MPH, WIND_MS };
enum PressureUnit { PRESS_HPA, PRESS_INHG };

inline TemperatureUnit temperatureUnit = TEMP_C;
inline WindUnit windUnit = WIND_KMH;
inline PressureUnit pressureUnit = PRESS_HPA;

// ---- Hardware: known-working V1 mapping --------------------------
inline constexpr int LCD_WIDTH  = 240;
inline constexpr int LCD_HEIGHT = 320;

inline constexpr int LCD_SCLK = 39;
inline constexpr int LCD_MOSI = 38;
inline constexpr int LCD_MISO = 40;
inline constexpr int LCD_DC   = 42;
inline constexpr int LCD_CS   = 45;
inline constexpr int LCD_RST  = -1;

inline constexpr int LCD_BACKLIGHT = 1;
inline constexpr int USER_BUTTON   = 0;
inline constexpr int BATTERY_ADC   = 4;
inline constexpr int SD_CS         = 21;

// ---- Features ----------------------------------------------------
inline constexpr bool ENABLE_SD_LOGGING = true;
inline constexpr bool ENABLE_AUTO_PAGE  = false;

// ---- Timing ------------------------------------------------------
inline constexpr uint32_t WEATHER_REFRESH_MS = 10UL * 60UL * 1000UL;
inline constexpr uint32_t AIR_REFRESH_MS     = 30UL * 60UL * 1000UL;
inline constexpr uint32_t HISTORY_LOG_MS     = 15UL * 60UL * 1000UL;
inline constexpr uint32_t WIFI_RETRY_MS      = 20UL * 1000UL;
inline constexpr uint32_t AUTO_PAGE_MS       = 30UL * 1000UL;
