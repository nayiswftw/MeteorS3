#pragma once

/*
 * Weather Station V4 — Core Data Types
 *
 * Plain data structures with no behavior and no dependencies on
 * hardware, UI, or networking.  Everything else builds on these.
 */

#include <Arduino.h>
#include <math.h>

// ================================================================
//  Limits
// ================================================================

constexpr int MAX_HOURLY  = 48;
constexpr int MAX_DAILY   = 10;
constexpr int MAX_ALERTS  = 10;
constexpr int MAX_HISTORY = 96;

// ================================================================
//  Unit Enums
// ================================================================

enum class TempUnit  : uint8_t { CELSIUS, FAHRENHEIT };
enum class WindUnit  : uint8_t { KMH, MPH, MS };
enum class PressUnit : uint8_t { HPA, INHG };

// ================================================================
//  Alert Severity
// ================================================================

enum class AlertSeverity : uint8_t { INFO, CAUTION, WARNING, DANGER };

// ================================================================
//  Ring Buffer — O(1) push, O(1) indexed read
// ================================================================

template<typename T, int Capacity>
struct RingBuffer {
    T   data[Capacity];
    int head  = 0;       // next write position
    int count = 0;       // number of valid items

    /// Push an item.  Overwrites the oldest entry when full.
    void push(const T& item) {
        data[head] = item;
        head = (head + 1) % Capacity;
        if (count < Capacity) ++count;
    }

    /// Read by logical index (0 = oldest, count-1 = newest).
    const T& at(int index) const {
        const int start = (head - count + Capacity) % Capacity;
        return data[(start + index) % Capacity];
    }

    /// Most recently pushed item.  Undefined if empty.
    const T& latest() const {
        return data[(head - 1 + Capacity) % Capacity];
    }

    int  size()  const { return count; }
    bool empty() const { return count == 0; }
    bool full()  const { return count == Capacity; }
    void reset()       { head = 0; count = 0; }
};

// ================================================================
//  Hourly Forecast Point
// ================================================================

struct HourData {
    char  time[6]       = "--:--";
    float temperature   = NAN;
    float apparent      = NAN;
    float humidity      = NAN;
    float dewPoint      = NAN;
    float pressure      = NAN;
    float cloudCover    = NAN;
    float visibility    = NAN;
    float uv            = NAN;
    float wind          = NAN;
    float gust          = NAN;
    float direction     = NAN;
    float precipitation = NAN;
    float rain          = NAN;
    int   rainChance    = 0;
    int   weatherCode   = -1;
};

// ================================================================
//  Daily Forecast Point
// ================================================================

struct DayData {
    char  date[11]      = "";
    float low           = NAN;
    float high          = NAN;
    float precipitation = NAN;
    float windMax       = NAN;
    float gustMax       = NAN;
    float uvMax         = NAN;
    int   rainChance    = 0;
    int   weatherCode   = -1;
    char  sunrise[6]    = "--:--";
    char  sunset[6]     = "--:--";
};

// ================================================================
//  Current Weather Snapshot
// ================================================================

struct WeatherData {
    bool valid = false;

    // Current conditions
    float temperature   = NAN;
    float apparent      = NAN;
    float humidity      = NAN;
    float dewPoint      = NAN;
    float pressure      = NAN;
    float wind          = NAN;
    float gust          = NAN;
    float direction     = NAN;
    float precipitation = NAN;
    float rain          = NAN;
    float visibility    = NAN;
    float cloudCover    = NAN;
    float uv            = NAN;

    // Pressure tendencies (computed from hourly history)
    float pressureDelta3h  = NAN;
    float pressureDelta6h  = NAN;
    float pressureDelta12h = NAN;

    int  weatherCode = -1;
    bool isDay       = true;

    // Hourly forecast (up to 48 hours from now)
    HourData hourly[MAX_HOURLY];
    int      hourlyCount = 0;

    // Daily forecast (up to 10 days)
    DayData daily[MAX_DAILY];
    int     dailyCount = 0;

    time_t fetchedAt = 0;
};

// ================================================================
//  Air Quality
// ================================================================

struct AirData {
    bool   valid       = false;
    float  pm25        = NAN;
    float  pm10        = NAN;
    float  ozone       = NAN;
    float  no2         = NAN;
    float  so2         = NAN;
    float  co          = NAN;
    int    usAqi       = -1;
    int    europeanAqi = -1;
    time_t fetchedAt   = 0;
};

// ================================================================
//  Local History Point (logged to SD card)
// ================================================================

struct HistoryPoint {
    time_t timestamp   = 0;
    float  temperature = NAN;
    float  humidity    = NAN;
    float  pressure    = NAN;
    float  aqi         = NAN;
};

// ================================================================
//  Weather Intelligence
// ================================================================

struct InsightData {
    char primary[64]   = "Waiting for forecast";
    char secondary[80] = "";

    int   nextRainHours     = -1;
    int   wettestHour       = -1;
    int   hottestHour       = -1;
    int   coldestHour       = -1;
    int   strongestWindHour = -1;

    float hottestTemp   = NAN;
    float coldestTemp   = NAN;
    float strongestGust = NAN;
};

// ================================================================
//  Alert
// ================================================================

struct AlertItem {
    char          title[40]  = "";
    char          detail[64] = "";
    AlertSeverity severity   = AlertSeverity::INFO;
};
