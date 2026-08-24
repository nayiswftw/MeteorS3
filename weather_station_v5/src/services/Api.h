#pragma once

/*
 * API Service — Open-Meteo Weather & Air Quality fetching and parsing.
 */

namespace svc {

void apiInit();
void apiService();
void apiForceRefresh();

bool refreshWeather();
bool refreshAir();

}  // namespace svc

