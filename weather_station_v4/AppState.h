#pragma once
#include "Models.h"

extern WeatherData weather;
extern AirData air;
extern InsightData insights;

extern AlertItem alerts[10];
extern int alertCount;

extern HistoryPoint historyPoints[96];
extern int historyCount;

extern bool wifiConnected;
extern bool sdAvailable;

extern uint32_t lastWeatherUpdate;
extern uint32_t lastAirUpdate;
extern uint32_t lastHistoryWrite;

void rebuildInsights();
void rebuildAlerts();
