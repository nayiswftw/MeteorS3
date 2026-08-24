#include "src/core/Weather.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace weather {

void rebuildInsights(const WeatherData& w,
                     const AirData&     a,
                     InsightData&       out) {
    out = InsightData();

    if (!w.valid || w.hourlyCount == 0) {
        snprintf(out.primary, sizeof(out.primary), "Waiting for forecast");
        snprintf(out.secondary, sizeof(out.secondary), "Current conditions will appear when data arrives");
        return;
    }

    float hottest = -999.0f;
    float coldest = 999.0f;
    float strongest = -1.0f;
    int wettestChance = -1;

    for (int i = 0; i < w.hourlyCount; ++i) {
        const HourData& h = w.hourly[i];

        if (!isnan(h.temperature) && h.temperature > hottest) {
            hottest = h.temperature;
            out.hottestHour = i;
            out.hottestTemp = h.temperature;
        }

        if (!isnan(h.temperature) && h.temperature < coldest) {
            coldest = h.temperature;
            out.coldestHour = i;
            out.coldestTemp = h.temperature;
        }

        if (!isnan(h.gust) && h.gust > strongest) {
            strongest = h.gust;
            out.strongestWindHour = i;
            out.strongestGust = h.gust;
        }

        if (h.rainChance > wettestChance) {
            wettestChance = h.rainChance;
            out.wettestHour = i;
        }

        if (out.nextRainHours < 0 && h.rainChance >= 60) {
            out.nextRainHours = i;
        }
    }

    if (w.weatherCode >= 95) {
        snprintf(out.primary, sizeof(out.primary), "Thunderstorm conditions");
        snprintf(out.secondary, sizeof(out.secondary), "Keep an eye on wind and lightning risk");
        return;
    }

    if (out.nextRainHours == 0) {
        snprintf(out.primary, sizeof(out.primary), "Rain likely now");
        snprintf(out.secondary, sizeof(out.secondary), "Umbrella recommended");
        return;
    }

    if (out.nextRainHours > 0 && out.nextRainHours <= 6) {
        snprintf(out.primary, sizeof(out.primary), "Rain likely in about %dh", out.nextRainHours);
        int peakChance = (out.wettestHour >= 0 && out.wettestHour < w.hourlyCount) 
                         ? w.hourly[out.wettestHour].rainChance : 0;
        snprintf(out.secondary, sizeof(out.secondary), "Peak chance %d%%", peakChance);
        return;
    }

    if (a.valid && a.usAqi >= 151) {
        snprintf(out.primary, sizeof(out.primary), "Air quality is poor");
        snprintf(out.secondary, sizeof(out.secondary), "Consider reducing prolonged outdoor activity");
        return;
    }

    if (w.uv >= 6.0f) {
        snprintf(out.primary, sizeof(out.primary), "High UV");
        snprintf(out.secondary, sizeof(out.secondary), "Sun protection recommended");
        return;
    }

    if (w.gust >= 50.0f) {
        snprintf(out.primary, sizeof(out.primary), "Windy conditions");
        snprintf(out.secondary, sizeof(out.secondary), "Strong gusts are possible");
        return;
    }

    if (!isnan(w.pressureDelta6h) && w.pressureDelta6h < -3.0f) {
        snprintf(out.primary, sizeof(out.primary), "Pressure falling");
        snprintf(out.secondary, sizeof(out.secondary), "Weather may become more unsettled");
        return;
    }

    snprintf(out.primary, sizeof(out.primary), "No major weather signal");
    snprintf(out.secondary, sizeof(out.secondary), "Conditions look relatively settled");
}

static void addAlert(AlertItem* out, int& count, int maxAlerts,
                     const char* title, const char* detail, AlertSeverity severity) {
    if (count >= maxAlerts || !out) return;
    snprintf(out[count].title, sizeof(out[count].title), "%s", title);
    snprintf(out[count].detail, sizeof(out[count].detail), "%s", detail);
    out[count].severity = severity;
    count++;
}

int rebuildAlerts(const WeatherData& w,
                  const AirData&     a,
                  AlertItem*         out,
                  int                maxAlerts) {
    int count = 0;
    if (!out || maxAlerts <= 0) return 0;

    if (!w.valid) {
        addAlert(out, count, maxAlerts, "Weather unavailable", "No current weather data", AlertSeverity::WARNING);
        return count;
    }

    if (w.weatherCode >= 95) {
        addAlert(out, count, maxAlerts, "Thunderstorm", "Thunderstorm conditions detected", AlertSeverity::DANGER);
    }

    if (w.temperature >= 40.0f) {
        addAlert(out, count, maxAlerts, "Extreme heat", "Temperature is at or above 40 C", AlertSeverity::DANGER);
    } else if (w.temperature >= 35.0f) {
        addAlert(out, count, maxAlerts, "High heat", "Hot outdoor conditions", AlertSeverity::WARNING);
    }

    if (w.gust >= 70.0f) {
        addAlert(out, count, maxAlerts, "Dangerous gusts", "Very strong wind gusts", AlertSeverity::DANGER);
    } else if (w.gust >= 50.0f) {
        addAlert(out, count, maxAlerts, "Strong wind", "Gusty outdoor conditions", AlertSeverity::WARNING);
    }

    if (w.hourlyCount > 0 && w.hourly[0].rainChance >= 80) {
        addAlert(out, count, maxAlerts, "Heavy rain risk", "Very high near-term rain probability", AlertSeverity::WARNING);
    }

    if (w.uv >= 8.0f) {
        addAlert(out, count, maxAlerts, "Very high UV", "Strong sun protection advised", AlertSeverity::WARNING);
    } else if (w.uv >= 6.0f) {
        addAlert(out, count, maxAlerts, "High UV", "Sun protection recommended", AlertSeverity::CAUTION);
    }

    if (a.valid && a.usAqi >= 201) {
        addAlert(out, count, maxAlerts, "Very poor air", "Air quality is very unhealthy", AlertSeverity::DANGER);
    } else if (a.valid && a.usAqi >= 151) {
        addAlert(out, count, maxAlerts, "Poor air quality", "Sensitive groups should limit exposure", AlertSeverity::WARNING);
    }

    if (!isnan(w.visibility)) {
        if (w.visibility <= 1000.0f) {
            addAlert(out, count, maxAlerts, "Low visibility", "Visibility is 1 km or less", AlertSeverity::WARNING);
        } else if (w.visibility <= 3000.0f) {
            addAlert(out, count, maxAlerts, "Reduced visibility", "Visibility is below 3 km", AlertSeverity::CAUTION);
        }
    }

    if (!isnan(w.pressureDelta3h) && w.pressureDelta3h <= -4.0f) {
        addAlert(out, count, maxAlerts, "Rapid pressure fall", "Pressure dropped quickly over 3 hours", AlertSeverity::WARNING);
    }

    if (!isnan(w.temperature) && w.temperature <= 2.0f) {
        addAlert(out, count, maxAlerts, "Cold conditions", "Near-freezing temperature", AlertSeverity::CAUTION);
    }

    if (count == 0) {
        addAlert(out, count, maxAlerts, "All clear", "No important weather alerts", AlertSeverity::INFO);
    }

    return count;
}

}  // namespace weather
