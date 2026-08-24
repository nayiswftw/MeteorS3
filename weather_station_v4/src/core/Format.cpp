#include "src/core/Format.h"
#include "src/Config.h"
#include <Arduino.h>
#include <stdio.h>
#include <math.h>

namespace fmt {

void temperature(char* buf, size_t len, float celsius) {
    if (!buf || len == 0) return;
    if (isnan(celsius)) {
        snprintf(buf, len, "--");
        return;
    }

    if (config::tempUnit == TempUnit::FAHRENHEIT) {
        float f = celsius * 9.0f / 5.0f + 32.0f;
        snprintf(buf, len, "%.0f F", f);
    } else {
        snprintf(buf, len, "%.0f C", celsius);
    }
}

void wind(char* buf, size_t len, float kmh) {
    if (!buf || len == 0) return;
    if (isnan(kmh)) {
        snprintf(buf, len, "--");
        return;
    }

    if (config::windUnit == WindUnit::MPH) {
        snprintf(buf, len, "%.0f mph", kmh * 0.621371f);
    } else if (config::windUnit == WindUnit::MS) {
        snprintf(buf, len, "%.1f m/s", kmh / 3.6f);
    } else {
        snprintf(buf, len, "%.0f km/h", kmh);
    }
}

void pressure(char* buf, size_t len, float hpa) {
    if (!buf || len == 0) return;
    if (isnan(hpa)) {
        snprintf(buf, len, "--");
        return;
    }

    if (config::pressUnit == PressUnit::INHG) {
        snprintf(buf, len, "%.2f inHg", hpa * 0.029529983f);
    } else {
        snprintf(buf, len, "%.0f hPa", hpa);
    }
}

void direction(char* buf, size_t len, float degrees) {
    if (!buf || len == 0) return;
    if (isnan(degrees)) {
        snprintf(buf, len, "--");
        return;
    }

    static const char* const directions[] = {
        "N", "NNE", "NE", "ENE",
        "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW",
        "W", "WNW", "NW", "NNW"
    };

    int index = (int)((degrees + 11.25f) / 22.5f) % 16;
    if (index < 0) index += 16;
    snprintf(buf, len, "%s", directions[index]);
}

void aqiCategory(char* buf, size_t len, int aqi) {
    if (!buf || len == 0) return;
    if (aqi < 0) {
        snprintf(buf, len, "--");
    } else if (aqi <= 50) {
        snprintf(buf, len, "Good");
    } else if (aqi <= 100) {
        snprintf(buf, len, "Moderate");
    } else if (aqi <= 150) {
        snprintf(buf, len, "Sensitive");
    } else if (aqi <= 200) {
        snprintf(buf, len, "Unhealthy");
    } else if (aqi <= 300) {
        snprintf(buf, len, "Very unhealthy");
    } else {
        snprintf(buf, len, "Hazardous");
    }
}

void weatherName(char* buf, size_t len, int code) {
    if (!buf || len == 0) return;

    const char* name = "Weather";
    if (code == 0) name = "Clear";
    else if (code == 1) name = "Mostly clear";
    else if (code == 2) name = "Partly cloudy";
    else if (code == 3) name = "Overcast";
    else if (code == 45 || code == 48) name = "Fog";
    else if (code >= 51 && code <= 57) name = "Drizzle";
    else if (code >= 61 && code <= 67) name = "Rain";
    else if (code >= 71 && code <= 77) name = "Snow";
    else if (code >= 80 && code <= 82) name = "Showers";
    else if (code >= 95) name = "Thunderstorm";

    snprintf(buf, len, "%s", name);
}

void age(char* buf, size_t len, time_t timestamp) {
    if (!buf || len == 0) return;
    if (timestamp <= 0) {
        snprintf(buf, len, "cached");
        return;
    }

    time_t now = time(nullptr);
    if (now <= 0 || now < timestamp) {
        snprintf(buf, len, "cached");
        return;
    }

    long minutes = (long)((now - timestamp) / 60);
    if (minutes < 1) {
        snprintf(buf, len, "live");
    } else if (minutes < 60) {
        snprintf(buf, len, "%ldm ago", minutes);
    } else {
        snprintf(buf, len, "%ldh ago", minutes / 60);
    }
}

void clock(char* buf, size_t len) {
    if (!buf || len == 0) return;
    struct tm now;
    if (!getLocalTime(&now, 20)) {
        snprintf(buf, len, "--:--");
        return;
    }
    strftime(buf, len, "%H:%M", &now);
}

void date(char* buf, size_t len) {
    if (!buf || len == 0) return;
    struct tm now;
    if (!getLocalTime(&now, 20)) {
        snprintf(buf, len, "--- -- ---");
        return;
    }
    strftime(buf, len, "%a, %d %b", &now);
}

void moonPhase(char* buf, size_t len) {
    if (!buf || len == 0) return;
    struct tm now;
    if (!getLocalTime(&now, 20)) {
        snprintf(buf, len, "Unknown");
        return;
    }

    int year = now.tm_year + 1900;
    int month = now.tm_mon + 1;
    int day = now.tm_mday;

    if (month < 3) {
        year--;
        month += 12;
    }

    long days = 365L * year + year / 4 - year / 100 + year / 400 +
                (153L * (month + 1)) / 5 + day - 730551L;

    double phase = fmod(days + 4.867, 29.530588853);
    if (phase < 0) phase += 29.530588853;

    double fraction = phase / 29.530588853;

    const char* phaseName = "New moon";
    if (fraction < 0.03) phaseName = "New moon";
    else if (fraction < 0.22) phaseName = "Waxing crescent";
    else if (fraction < 0.28) phaseName = "First quarter";
    else if (fraction < 0.47) phaseName = "Waxing gibbous";
    else if (fraction < 0.53) phaseName = "Full moon";
    else if (fraction < 0.72) phaseName = "Waning gibbous";
    else if (fraction < 0.78) phaseName = "Last quarter";
    else if (fraction < 0.97) phaseName = "Waning crescent";

    snprintf(buf, len, "%s", phaseName);
}

}  // namespace fmt
