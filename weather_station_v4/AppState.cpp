#include "AppState.h"
#include <math.h>

WeatherData weather;
AirData air;
InsightData insights;

AlertItem alerts[10];
int alertCount = 0;

HistoryPoint historyPoints[96];
int historyCount = 0;

bool wifiConnected = false;
bool sdAvailable = false;

uint32_t lastWeatherUpdate = 0;
uint32_t lastAirUpdate = 0;
uint32_t lastHistoryWrite = 0;

static void addAlert(const String &title,
                     const String &detail,
                     AlertSeverity severity) {
  if (alertCount >= 10) return;

  alerts[alertCount].title = title;
  alerts[alertCount].detail = detail;
  alerts[alertCount].severity = severity;
  alertCount++;
}

void rebuildInsights() {
  insights = InsightData();

  if (!weather.valid || weather.hourlyCount == 0) {
    insights.primary = "Waiting for forecast";
    insights.secondary = "Current conditions will appear when data arrives";
    return;
  }

  float hottest = -999;
  float coldest = 999;
  float strongest = -1;

  int wettestChance = -1;

  for (int i = 0; i < weather.hourlyCount; i++) {
    const HourData &h = weather.hourly[i];

    if (!isnan(h.temperature) && h.temperature > hottest) {
      hottest = h.temperature;
      insights.hottestHour = i;
      insights.hottestTemperature = h.temperature;
    }

    if (!isnan(h.temperature) && h.temperature < coldest) {
      coldest = h.temperature;
      insights.coldestHour = i;
      insights.coldestTemperature = h.temperature;
    }

    if (!isnan(h.gust) && h.gust > strongest) {
      strongest = h.gust;
      insights.strongestWindHour = i;
      insights.strongestGust = h.gust;
    }

    if (h.rainChance > wettestChance) {
      wettestChance = h.rainChance;
      insights.wettestHour = i;
    }

    if (insights.nextRainHours < 0 && h.rainChance >= 60) {
      insights.nextRainHours = i;
    }
  }

  if (weather.weatherCode >= 95) {
    insights.primary = "Thunderstorm conditions";
    insights.secondary = "Keep an eye on wind and lightning risk";
    return;
  }

  if (insights.nextRainHours == 0) {
    insights.primary = "Rain likely now";
    insights.secondary = "Umbrella recommended";
    return;
  }

  if (insights.nextRainHours > 0 && insights.nextRainHours <= 6) {
    insights.primary =
      "Rain likely in about " + String(insights.nextRainHours) + "h";

    insights.secondary =
      "Peak chance " +
      String(weather.hourly[insights.wettestHour].rainChance) + "%";
    return;
  }

  if (air.valid && air.usAqi >= 151) {
    insights.primary = "Air quality is poor";
    insights.secondary = "Consider reducing prolonged outdoor activity";
    return;
  }

  if (weather.uv >= 6) {
    insights.primary = "High UV";
    insights.secondary = "Sun protection recommended";
    return;
  }

  if (weather.gust >= 50) {
    insights.primary = "Windy conditions";
    insights.secondary = "Strong gusts are possible";
    return;
  }

  if (weather.pressureDelta6h < -3) {
    insights.primary = "Pressure falling";
    insights.secondary = "Weather may become more unsettled";
    return;
  }

  insights.primary = "No major weather signal";
  insights.secondary = "Conditions look relatively settled";
}

void rebuildAlerts() {
  alertCount = 0;

  if (!weather.valid) {
    addAlert("Weather unavailable",
             "No current weather data",
             ALERT_WARNING);
    return;
  }

  if (weather.weatherCode >= 95) {
    addAlert("Thunderstorm",
             "Thunderstorm conditions detected",
             ALERT_DANGER);
  }

  if (weather.temperature >= 40) {
    addAlert("Extreme heat",
             "Temperature is at or above 40 C",
             ALERT_DANGER);
  } else if (weather.temperature >= 35) {
    addAlert("High heat",
             "Hot outdoor conditions",
             ALERT_WARNING);
  }

  if (weather.gust >= 70) {
    addAlert("Dangerous gusts",
             "Very strong wind gusts",
             ALERT_DANGER);
  } else if (weather.gust >= 50) {
    addAlert("Strong wind",
             "Gusty outdoor conditions",
             ALERT_WARNING);
  }

  if (weather.hourlyCount > 0 &&
      weather.hourly[0].rainChance >= 80) {
    addAlert("Heavy rain risk",
             "Very high near-term rain probability",
             ALERT_WARNING);
  }

  if (weather.uv >= 8) {
    addAlert("Very high UV",
             "Strong sun protection advised",
             ALERT_WARNING);
  } else if (weather.uv >= 6) {
    addAlert("High UV",
             "Sun protection recommended",
             ALERT_CAUTION);
  }

  if (air.valid && air.usAqi >= 201) {
    addAlert("Very poor air",
             "Air quality is very unhealthy",
             ALERT_DANGER);
  } else if (air.valid && air.usAqi >= 151) {
    addAlert("Poor air quality",
             "Sensitive groups should limit exposure",
             ALERT_WARNING);
  }

  if (weather.visibility <= 1000) {
    addAlert("Low visibility",
             "Visibility is 1 km or less",
             ALERT_WARNING);
  } else if (weather.visibility <= 3000) {
    addAlert("Reduced visibility",
             "Visibility is below 3 km",
             ALERT_CAUTION);
  }

  if (weather.pressureDelta3h <= -4) {
    addAlert("Rapid pressure fall",
             "Pressure dropped quickly over 3 hours",
             ALERT_WARNING);
  }

  if (weather.temperature <= 2) {
    addAlert("Cold conditions",
             "Near-freezing temperature",
             ALERT_CAUTION);
  }

  if (alertCount == 0) {
    addAlert("All clear",
             "No important weather alerts",
             ALERT_INFO);
  }
}
