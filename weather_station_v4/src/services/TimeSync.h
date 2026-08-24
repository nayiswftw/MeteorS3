#pragma once

/*
 * TimeSync Service — NTP configuration and synchronization status.
 */

namespace svc {

void timeSyncInit();
bool isTimeSynced();

}  // namespace svc
