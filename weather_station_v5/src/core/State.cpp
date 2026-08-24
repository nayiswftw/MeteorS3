#include "src/core/State.h"
#include "src/core/Weather.h"
#include <string.h>

// ================================================================
//  Private storage — only accessible through the API below
// ================================================================

static WeatherData                          s_weather;
static AirData                              s_air;
static InsightData                          s_insights;
static AlertItem                            s_alerts[MAX_ALERTS];
static int                                  s_alertCount = 0;
static RingBuffer<HistoryPoint, MAX_HISTORY> s_history;
static RuntimeConfig                        s_config;
static SystemTelemetry                      s_telemetry;
static char                                 s_customAlert[64] = "";

static bool     s_online           = false;
static bool     s_sdReady          = false;
static bool     s_apMode           = false;
static uint32_t s_lastWeatherMs    = 0;
static uint32_t s_lastAirMs        = 0;
static uint32_t s_lastHistoryMs    = 0;

static SemaphoreHandle_t s_mutex   = nullptr;

// ================================================================
//  Internal helper
// ================================================================

static void rebuildDerived() {
    weather::rebuildInsights(s_weather, s_air, s_insights);
    s_alertCount = weather::rebuildAlerts(
        s_weather, s_air, s_alerts, MAX_ALERTS);
}

// ================================================================
//  Mutex Lock / Unlock
// ================================================================

namespace state {

void init() {
    if (!s_mutex) {
        s_mutex = xSemaphoreCreateMutex();
    }
}

void lock() {
    if (s_mutex) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }
}

void unlock() {
    if (s_mutex) {
        xSemaphoreGive(s_mutex);
    }
}

// ================================================================
//  Read accessors
// ================================================================

const WeatherData&     weather()      { return s_weather; }
const AirData&         air()          { return s_air; }
const InsightData&     insights()     { return s_insights; }
const AlertItem*       alerts()       { return s_alerts; }
int                    alertCount()   { return s_alertCount; }
const RuntimeConfig&   config()       { return s_config; }
const SystemTelemetry& telemetry()    { return s_telemetry; }
const char*            customAlert()  { return s_customAlert; }

const RingBuffer<HistoryPoint, MAX_HISTORY>& history() {
    return s_history;
}

bool     isOnline()            { return s_online; }
bool     isSdReady()           { return s_sdReady; }
bool     isApMode()            { return s_apMode; }
uint32_t lastWeatherUpdate()   { return s_lastWeatherMs; }
uint32_t lastAirUpdate()       { return s_lastAirMs; }
uint32_t lastHistoryWrite()    { return s_lastHistoryMs; }

// ================================================================
//  Write accessors
// ================================================================

void setWeather(const WeatherData& data) {
    lock();
    s_weather = data;
    rebuildDerived();
    unlock();
}

void setAir(const AirData& data) {
    lock();
    s_air = data;
    rebuildDerived();
    unlock();
}

void setOnline(bool connected)        { lock(); s_online = connected; unlock(); }
void setSdReady(bool ready)           { lock(); s_sdReady = ready; unlock(); }
void setApMode(bool apMode)           { lock(); s_apMode = apMode; unlock(); }
void setLastWeatherUpdate(uint32_t ms){ lock(); s_lastWeatherMs = ms; unlock(); }
void setLastAirUpdate(uint32_t ms)    { lock(); s_lastAirMs = ms; unlock(); }
void setLastHistoryWrite(uint32_t ms) { lock(); s_lastHistoryMs = ms; unlock(); }

void setConfig(const RuntimeConfig& cfg) {
    lock();
    s_config = cfg;
    unlock();
}

void setTelemetry(const SystemTelemetry& telem) {
    lock();
    s_telemetry = telem;
    unlock();
}

void setCustomAlert(const char* alertText) {
    lock();
    if (alertText) {
        snprintf(s_customAlert, sizeof(s_customAlert), "%s", alertText);
    } else {
        s_customAlert[0] = '\0';
    }
    unlock();
}

void clearCustomAlert() {
    lock();
    s_customAlert[0] = '\0';
    unlock();
}

void pushHistory(const HistoryPoint& point) {
    lock();
    s_history.push(point);
    unlock();
}

void loadHistory(const HistoryPoint* points, int count) {
    lock();
    s_history.reset();
    for (int i = 0; i < count; ++i) {
        s_history.push(points[i]);
    }
    unlock();
}

}  // namespace state

