#pragma once

/*
 * Unit-aware formatting helpers.
 *
 * Every function writes into a caller-provided char buffer —
 * zero heap allocations.  The active unit preference is read
 * from config:: at call time.
 */

#include <stddef.h>
#include <time.h>

namespace fmt {

/// "23°C" / "73°F"
void temperature(char* buf, size_t len, float celsius);

/// "23°" / "73°" (compact without letter)
void tempShort(char* buf, size_t len, float celsius);

/// "18 km/h" / "11 mph" / "5.0 m/s"
void wind(char* buf, size_t len, float kmh);

/// "1013 hPa" / "29.92 inHg"
void pressure(char* buf, size_t len, float hpa);

/// 16-point compass: "N", "NNE", "NE", ...
void direction(char* buf, size_t len, float degrees);

/// "Good", "Moderate", "Sensitive", ...
void aqiCategory(char* buf, size_t len, int aqi);

/// WMO code → short name: "Clear", "Rain", "Fog", ...
void weatherName(char* buf, size_t len, int code);

/// "live", "3m ago", "2h ago", "cached"
void age(char* buf, size_t len, time_t timestamp);

/// "HH:MM" from local clock.  "--:--" if NTP not synced.
void clock(char* buf, size_t len);

/// "Mon, 23 Aug" from local clock.
void date(char* buf, size_t len);

/// "Full moon", "Waxing crescent", etc.
void moonPhase(char* buf, size_t len);

}  // namespace fmt
