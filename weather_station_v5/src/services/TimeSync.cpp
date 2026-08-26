#include "src/services/TimeSync.h"
#include "src/core/State.h"
#include "src/Config.h"
#include <Arduino.h>
#include <time.h>

namespace svc {

static const char* tzNameToPosix(const char* tz) {
    if (!tz || tz[0] == '\0') return "IST-5:30";
    if (strstr(tz, "Kolkata") || strstr(tz, "Calcutta") || strstr(tz, "India") || strstr(tz, "Asia/Kolkata")) return "IST-5:30";
    if (strstr(tz, "New_York") || strstr(tz, "Eastern")) return "EST5EDT,M3.2.0,M11.1.0";
    if (strstr(tz, "Chicago") || strstr(tz, "Central"))  return "CST6CDT,M3.2.0,M11.1.0";
    if (strstr(tz, "Denver") || strstr(tz, "Mountain"))  return "MST7MDT,M3.2.0,M11.1.0";
    if (strstr(tz, "Los_Angeles") || strstr(tz, "Pacific")) return "PST8PDT,M3.2.0,M11.1.0";
    if (strstr(tz, "London") || strstr(tz, "GMT"))       return "GMT0BST,M3.5.0/1,M10.5.0";
    if (strstr(tz, "Berlin") || strstr(tz, "Paris"))     return "CET-1CEST,M3.5.0,M10.5.0/3";
    if (strstr(tz, "Dubai"))                             return "GST-4";
    if (strstr(tz, "Singapore"))                         return "SGT-8";
    if (strstr(tz, "Tokyo"))                             return "JST-9";
    if (strstr(tz, "Sydney"))                            return "AEST-10AEDT,M10.1.0,M4.1.0/3";
    if (strstr(tz, "UTC"))                               return "UTC0";
    return tz;
}

void timeSyncInit() {
    state::lock();
    const char* tz = state::config().timezone;
    const char* tzStr = (tz && tz[0] != '\0') ? tz : config::TIMEZONE;
    state::unlock();

    const char* posixTz = tzNameToPosix(tzStr);

    configTzTime(
        posixTz,
        "pool.ntp.org",
        "time.google.com",
        "time.cloudflare.com"
    );
    Serial.printf("[time] configured for timezone: %s (%s)\n", tzStr, posixTz);
}


bool isTimeSynced() {
    time_t now = time(nullptr);
    return (now > 1672531199); // after 2023-01-01
}

}  // namespace svc
