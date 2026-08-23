#include "core/State.h"
#include "core/Weather.h"

// ================================================================
//  Private storage — only accessible through the API below
// ================================================================

static WeatherData                          s_weather;
static AirData                              s_air;
static InsightData                          s_insights;
static AlertItem                            s_alerts[MAX_ALERTS];
static int                                  s_alertCount = 0;
static RingBuffer<HistoryPoint, MAX_HISTORY> s_history;

static bool     s_online           = false;
static bool     s_sdReady          = false;
static uint32_t s_lastWeatherMs    = 0;
static uint32_t s_lastAirMs        = 0;
static uint32_t s_lastHistoryMs    = 0;

// ================================================================
//  Internal helper
// ================================================================

static void rebuildDerived() {
    weather::rebuildInsights(s_weather, s_air, s_insights);
    s_alertCount = weather::rebuildAlerts(
        s_weather, s_air, s_alerts, MAX_ALERTS);
}

// ================================================================
//  Read accessors
// ================================================================

namespace state {

const WeatherData& weather()      { return s_weather; }
const AirData&     air()          { return s_air; }
const InsightData& insights()     { return s_insights; }
const AlertItem*   alerts()       { return s_alerts; }
int                alertCount()   { return s_alertCount; }

const RingBuffer<HistoryPoint, MAX_HISTORY>& history() {
    return s_history;
}

bool     isOnline()            { return s_online; }
bool     isSdReady()           { return s_sdReady; }
uint32_t lastWeatherUpdate()   { return s_lastWeatherMs; }
uint32_t lastAirUpdate()       { return s_lastAirMs; }
uint32_t lastHistoryWrite()    { return s_lastHistoryMs; }

// ================================================================
//  Write accessors
// ================================================================

void setWeather(const WeatherData& data) {
    s_weather = data;
    rebuildDerived();
}

void setAir(const AirData& data) {
    s_air = data;
    rebuildDerived();
}

void setOnline(bool connected)        { s_online = connected; }
void setSdReady(bool ready)           { s_sdReady = ready; }
void setLastWeatherUpdate(uint32_t ms){ s_lastWeatherMs = ms; }
void setLastAirUpdate(uint32_t ms)    { s_lastAirMs = ms; }
void setLastHistoryWrite(uint32_t ms) { s_lastHistoryMs = ms; }

void pushHistory(const HistoryPoint& point) {
    s_history.push(point);
}

void loadHistory(const HistoryPoint* points, int count) {
    s_history.reset();
    for (int i = 0; i < count; ++i) {
        s_history.push(points[i]);
    }
}

}  // namespace state
