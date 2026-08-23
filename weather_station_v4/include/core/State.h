#pragma once

/*
 * Centralized application state.
 *
 * All mutable data lives here behind accessor functions.
 * Read access returns const references — nobody else can mutate.
 * Write access is through explicit set/push functions.
 */

#include "core/Types.h"

namespace state {

// ---- Read-only accessors ------------------------------------------

const WeatherData&                     weather();
const AirData&                         air();
const InsightData&                     insights();
const AlertItem*                       alerts();      // array of up to MAX_ALERTS
int                                    alertCount();
const RingBuffer<HistoryPoint, MAX_HISTORY>& history();

bool     isOnline();
bool     isSdReady();
uint32_t lastWeatherUpdate();
uint32_t lastAirUpdate();
uint32_t lastHistoryWrite();

// ---- Write accessors ----------------------------------------------

/// Replace weather data and rebuild derived intelligence.
void setWeather(const WeatherData& data);

/// Replace air quality data and rebuild derived intelligence.
void setAir(const AirData& data);

void setOnline(bool connected);
void setSdReady(bool ready);
void setLastWeatherUpdate(uint32_t ms);
void setLastAirUpdate(uint32_t ms);
void setLastHistoryWrite(uint32_t ms);

/// Append a point to the history ring buffer.
void pushHistory(const HistoryPoint& point);

/// Bulk-load history (e.g. from SD on boot).
void loadHistory(const HistoryPoint* points, int count);

}  // namespace state
