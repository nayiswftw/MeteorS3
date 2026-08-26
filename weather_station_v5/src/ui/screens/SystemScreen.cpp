#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/hal/Battery.h"
#include "src/hal/Imu.h"
#include "src/services/Network.h"
#include "src/services/Mqtt.h"
#include "src/core/State.h"
#include <Arduino.h>
#include <stdio.h>

static void render(lv_obj_t* root) {
    int batPct   = hal::readBatteryPercent();
    float batV   = hal::readBatteryVoltage();
    bool online  = state::isOnline();
    bool apMode  = state::isApMode();
    int rssi     = svc::wifiRssi();

    state::lock();
    SystemTelemetry telem = state::telemetry();
    bool gesturesEn = state::config().enableGestures;
    state::unlock();

    // Battery Gauge
    char batBuf[16];
    snprintf(batBuf, sizeof(batBuf), "%d%%", batPct);
    ui::gauge(root, 10, 54, 98, batPct, 0, 100, batBuf, "Battery",
              (batPct <= 20) ? CLR_RED : CLR_GREEN);

    // WiFi RSSI Gauge
    int signalVal = online ? constrain(rssi + 100, 0, 70) : 0;
    char rssiBuf[16];
    if (online) {
        snprintf(rssiBuf, sizeof(rssiBuf), "%d", rssi);
    } else {
        snprintf(rssiBuf, sizeof(rssiBuf), apMode ? "AP" : "--");
    }
    ui::gauge(root, 132, 54, 98, signalVal, 0, 70, rssiBuf, apMode ? "Setup AP" : "WiFi dBm",
              online ? CLR_CYAN : (apMode ? CLR_YELLOW : CLR_DIM));

    // HEAP Card
    char heapBuf[16];
    snprintf(heapBuf, sizeof(heapBuf), "%u KB", ESP.getFreeHeap() / 1024);
    ui::metricCard(root, 10, 162, layout::CARD_W_HALF, 54, "HEAP", heapBuf, "Free", CLR_CYAN);

    // PSRAM Card
    char psramBuf[16];
    const char* psramCap = "Not found";
    if (psramFound()) {
        snprintf(psramBuf, sizeof(psramBuf), "%u KB", ESP.getFreePsram() / 1024);
        psramCap = "Free";
    } else {
        snprintf(psramBuf, sizeof(psramBuf), "--");
    }
    ui::metricCard(root, 125, 162, layout::CARD_W_HALF, 54, "PSRAM", psramBuf, psramCap, CLR_PURPLE);

    // IP & WEB PORTAL Card
    char statusValBuf[32];
    char statusCapBuf[64];
    if (apMode) {
        snprintf(statusValBuf, sizeof(statusValBuf), "192.168.4.1");
        snprintf(statusCapBuf, sizeof(statusCapBuf), "Connect 'WeatherStation-Setup'");
    } else if (online) {
        snprintf(statusValBuf, sizeof(statusValBuf), "%s", telem.ipAddress);
        snprintf(statusCapBuf, sizeof(statusCapBuf), "SD %s  •  IMU %s  •  MQTT %s",
                 state::isSdReady() ? "OK" : "--",
                 hal::isImuAvailable() ? (gesturesEn ? "OK" : "Off") : "N/A",
                 svc::isMqttConnected() ? "ON" : "OFF");
    } else {
        snprintf(statusValBuf, sizeof(statusValBuf), "DISCONNECTED");
        snprintf(statusCapBuf, sizeof(statusCapBuf), "%.2f V  •  IMU %s  •  SD %s",
                 batV, hal::isImuAvailable() ? (gesturesEn ? "Ready" : "Off") : "N/A",
                 state::isSdReady() ? "OK" : "--");
    }

    ui::metricCard(root, 10, 226, layout::CARD_W_FULL, 64, "NETWORK & SYSTEM",
                   statusValBuf, statusCapBuf,
                   online ? CLR_GREEN : (apMode ? CLR_YELLOW : CLR_RED));
}

static const bool s_reg = screen::registerScreen(
    SCREEN_SYSTEM,
    { "SYSTEM & TELEMETRY", CLR_SYSTEM_TOP, render }
);

