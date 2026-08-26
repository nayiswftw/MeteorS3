#pragma once

/*
 * Weather Station V4 — Configuration
 *
 * Edit this file to set your WiFi credentials, location, units,
 * and board-specific pin assignments.
 */

#include <Arduino.h>
#include "src/core/Types.h"

namespace config {

// ================================================================
//  USER SETTINGS — edit these
// ================================================================

constexpr char WIFI_SSID[]     = "Xiaomi 11i HyperCharge";
constexpr char WIFI_PASSWORD[] = "12345678";

constexpr double LATITUDE  = 30.000000;
constexpr double LONGITUDE = 75.000000;

constexpr char LOCATION_NAME[] = "HOME";
constexpr char TIMEZONE[]      = "Asia/Kolkata";

// ================================================================
//  UNITS
// ================================================================

inline TempUnit  tempUnit  = TempUnit::CELSIUS;
inline WindUnit  windUnit  = WindUnit::KMH;
inline PressUnit pressUnit = PressUnit::HPA;

// ================================================================
//  HARDWARE PINS — Waveshare ESP32-S3-LCD-2
// ================================================================

// LCD (ST7789T3 via SPI)
constexpr int LCD_WIDTH  = 240;
constexpr int LCD_HEIGHT = 320;
constexpr int LCD_SCLK   = 39;
constexpr int LCD_MOSI   = 38;
constexpr int LCD_MISO   = 40;
constexpr int LCD_DC     = 42;
constexpr int LCD_CS     = 45;
constexpr int LCD_RST    = -1;   // hardware-reset only
constexpr int LCD_BL     = 1;    // backlight PWM

// Peripherals
constexpr int BUTTON_PIN  = 0;   // BOOT button
constexpr int BATTERY_ADC = 4;   // voltage-divider tap
constexpr int SD_CS       = 21;  // SD card chip-select

// IMU (QMI8658 via I2C) — Waveshare ESP32-S3-LCD-2 / Touch-LCD-2 official pins
constexpr int     IMU_SDA  = 48;
constexpr int     IMU_SCL  = 47;
constexpr uint8_t IMU_ADDR = 0x6B;

// ================================================================
//  FEATURES
// ================================================================

constexpr bool ENABLE_SD        = true;
constexpr bool ENABLE_AUTO_PAGE = false;

// ================================================================
//  TIMING
// ================================================================

constexpr uint32_t WEATHER_INTERVAL_MS = 10UL * 60 * 1000;  // 10 min
constexpr uint32_t AIR_INTERVAL_MS     = 30UL * 60 * 1000;  // 30 min
constexpr uint32_t HISTORY_INTERVAL_MS = 15UL * 60 * 1000;  // 15 min
constexpr uint32_t AUTO_PAGE_MS        = 30UL * 1000;        // 30 sec

// WiFi retry — exponential backoff between these bounds
constexpr uint32_t WIFI_RETRY_MIN_MS = 5UL  * 1000;         //  5 sec
constexpr uint32_t WIFI_RETRY_MAX_MS = 60UL * 1000;         // 60 sec

// ================================================================
//  DISPLAY — LVGL buffer tuning & DMA Acceleration
// ================================================================

// Number of horizontal lines per partial-render stripe.
// 64 lines × 240 px × 2 bytes = 30 720 bytes per buffer (×2 double-buffered DMA).
constexpr int LVGL_BUFFER_LINES = 64;

}  // namespace config
