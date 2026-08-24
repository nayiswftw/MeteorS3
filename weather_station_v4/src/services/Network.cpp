#include "src/services/Network.h"
#include "src/Config.h"
#include "src/core/State.h"
#include <WiFi.h>

namespace svc {

static WifiState s_state       = WifiState::IDLE;
static uint32_t  s_stateTimer  = 0;
static uint32_t  s_backoffMs   = config::WIFI_RETRY_MIN_MS;

void networkInit() {
    if (strlen(config::WIFI_SSID) == 0 || strcmp(config::WIFI_SSID, "YOUR_WIFI_NAME") == 0) {
        Serial.println("[wifi] credentials not configured");
        s_state = WifiState::IDLE;
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    Serial.printf("[wifi] connecting to %s\n", config::WIFI_SSID);
    WiFi.begin(config::WIFI_SSID, config::WIFI_PASSWORD);
    s_state      = WifiState::CONNECTING;
    s_stateTimer = millis();
}

void networkService() {
    bool connected = (WiFi.status() == WL_CONNECTED);

    switch (s_state) {
        case WifiState::IDLE:
            break;

        case WifiState::CONNECTING:
            if (connected) {
                s_state     = WifiState::CONNECTED;
                s_backoffMs = config::WIFI_RETRY_MIN_MS;
                state::setOnline(true);
                Serial.printf("[wifi] connected! IP: %s\n", WiFi.localIP().toString().c_str());
            } else if (millis() - s_stateTimer >= 15000) { // 15s timeout
                Serial.println("[wifi] connection timed out");
                WiFi.disconnect();
                s_state      = WifiState::BACKOFF;
                s_stateTimer = millis();
                state::setOnline(false);
            }
            break;

        case WifiState::CONNECTED:
            if (!connected) {
                Serial.println("[wifi] connection lost");
                s_state      = WifiState::BACKOFF;
                s_stateTimer = millis();
                state::setOnline(false);
            }
            break;

        case WifiState::BACKOFF:
            if (millis() - s_stateTimer >= s_backoffMs) {
                Serial.printf("[wifi] retrying connection (backoff %lu ms)...\n", s_backoffMs);
                WiFi.disconnect();
                WiFi.begin(config::WIFI_SSID, config::WIFI_PASSWORD);
                s_state      = WifiState::CONNECTING;
                s_stateTimer = millis();

                // Increase backoff for next time (exponential, capped)
                s_backoffMs = min(s_backoffMs * 2, config::WIFI_RETRY_MAX_MS);
            }
            break;
    }
}

bool isWifiConnected() {
    return (s_state == WifiState::CONNECTED);
}

int wifiRssi() {
    return isWifiConnected() ? WiFi.RSSI() : -100;
}

WifiState wifiState() {
    return s_state;
}

}  // namespace svc
