#pragma once

/*
 * Network Service — WiFi state machine with exponential backoff.
 */

#include <Arduino.h>

namespace svc {

enum class WifiState : uint8_t {
    IDLE,
    CONNECTING,
    CONNECTED,
    BACKOFF
};

void networkInit();
void networkService();

bool        isWifiConnected();
int         wifiRssi();
WifiState   wifiState();

}  // namespace svc
