#include "src/services/TimeSync.h"
#include "src/core/State.h"
#include "src/Config.h"
#include <Arduino.h>
#include <time.h>

namespace svc {

void timeSyncInit() {
    state::lock();
    const char* tz = state::config().timezone;
    const char* tzStr = (tz && tz[0] != '\0') ? tz : config::TIMEZONE;
    state::unlock();

    configTzTime(
        tzStr,
        "pool.ntp.org",
        "time.google.com",
        "time.cloudflare.com"
    );
    Serial.printf("[time] configured for timezone: %s\n", tzStr);
}


bool isTimeSynced() {
    time_t now = time(nullptr);
    return (now > 1672531199); // after 2023-01-01
}

}  // namespace svc
