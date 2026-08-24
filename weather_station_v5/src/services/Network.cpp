#include "src/services/Network.h"
#include "src/Config.h"
#include "src/core/State.h"
#include <WiFi.h>
#include <DNSServer.h>

namespace svc {

static WifiState s_state       = WifiState::IDLE;
static uint32_t  s_stateTimer  = 0;
static uint32_t  s_backoffMs   = config::WIFI_RETRY_MIN_MS;
static int       s_failCount   = 0;
static DNSServer s_dnsServer;

void networkStartApMode() {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP("WeatherStation-Setup", "12345678");
    IPAddress apIP = WiFi.softAPIP();

    // Start Captive Portal DNS Server (redirect all *.com, etc. to 192.168.4.1)
    s_dnsServer.start(53, "*", apIP);

    s_state = WifiState::AP_MODE;
    state::setOnline(false);
    state::setApMode(true);

    SystemTelemetry telem = state::telemetry();
    telem.isApMode = true;
    snprintf(telem.ipAddress, sizeof(telem.ipAddress), "%s", apIP.toString().c_str());
    state::setTelemetry(telem);

    Serial.printf("[wifi] AP mode active! SSID: WeatherStation-Setup, IP: %s\n", apIP.toString().c_str());
}

bool networkIsApMode() {
    return (s_state == WifiState::AP_MODE);
}

void networkInit() {
    state::lock();
    const char* ssid = state::config().wifiSsid;
    const char* pass = state::config().wifiPassword;
    state::unlock();

    if (strlen(ssid) == 0 || strcmp(ssid, "Your_WiFi_SSID") == 0) {
        Serial.println("[wifi] no credentials configured, launching AP setup mode...");
        networkStartApMode();
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    Serial.printf("[wifi] connecting to %s...\n", ssid);
    WiFi.begin(ssid, pass);
    s_state      = WifiState::CONNECTING;
    s_stateTimer = millis();
    s_failCount  = 0;
}

void networkService() {
    if (s_state == WifiState::AP_MODE) {
        s_dnsServer.processNextRequest();
        return;
    }

    bool connected = (WiFi.status() == WL_CONNECTED);

    switch (s_state) {
        case WifiState::IDLE:
            break;

        case WifiState::CONNECTING:
            if (connected) {
                s_state     = WifiState::CONNECTED;
                s_backoffMs = config::WIFI_RETRY_MIN_MS;
                s_failCount = 0;
                state::setOnline(true);
                state::setApMode(false);

                SystemTelemetry telem = state::telemetry();
                telem.isApMode = false;
                telem.wifiRssi = WiFi.RSSI();
                snprintf(telem.ipAddress, sizeof(telem.ipAddress), "%s", WiFi.localIP().toString().c_str());
                state::setTelemetry(telem);

                Serial.printf("[wifi] connected! IP: %s (RSSI: %d dBm)\n",
                              WiFi.localIP().toString().c_str(), WiFi.RSSI());
            } else if (millis() - s_stateTimer >= 15000) { // 15s timeout
                Serial.println("[wifi] connection timed out");
                WiFi.disconnect();
                s_failCount++;

                if (s_failCount >= 4) {
                    Serial.println("[wifi] multiple failures, opening AP setup mode");
                    networkStartApMode();
                    return;
                }

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
            } else {
                // Periodically update telemetry RSSI
                SystemTelemetry telem = state::telemetry();
                telem.wifiRssi = WiFi.RSSI();
                snprintf(telem.ipAddress, sizeof(telem.ipAddress), "%s", WiFi.localIP().toString().c_str());
                state::setTelemetry(telem);
            }
            break;

        case WifiState::BACKOFF:
            if (millis() - s_stateTimer >= s_backoffMs) {
                state::lock();
                const char* ssid = state::config().wifiSsid;
                const char* pass = state::config().wifiPassword;
                state::unlock();

                Serial.printf("[wifi] retrying %s (backoff %lu ms)...\n", ssid, s_backoffMs);
                WiFi.disconnect();
                WiFi.begin(ssid, pass);
                s_state      = WifiState::CONNECTING;
                s_stateTimer = millis();

                s_backoffMs = min(s_backoffMs * 2, config::WIFI_RETRY_MAX_MS);
            }
            break;

        case WifiState::AP_MODE:
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

