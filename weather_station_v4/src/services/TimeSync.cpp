#include "src/services/TimeSync.h"
#include "src/Config.h"
#include <Arduino.h>
#include <time.h>

namespace svc {

void timeSyncInit() {
    configTzTime(
        config::TIMEZONE,
        "pool.ntp.org",
        "time.google.com",
        "time.cloudflare.com"
    );
    Serial.printf("[time] configured for timezone: %s\n", config::TIMEZONE);
}

bool isTimeSynced() {
    time_t now = time(nullptr);
    return (now > 1672531199); // after 2023-01-01
}

}  // namespace svc
