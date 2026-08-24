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
    BACKOFF,
    AP_MODE
};

void networkInit();
void networkService();

void networkStartApMode();
bool networkIsApMode();

bool        isWifiConnected();
int         wifiRssi();
WifiState   wifiState();

}  // namespace svc

