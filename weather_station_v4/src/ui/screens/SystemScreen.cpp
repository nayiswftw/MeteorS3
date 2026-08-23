#include "ui/Screen.h"
#include "ui/Widgets.h"
#include "ui/Theme.h"
#include "hal/Battery.h"
#include "services/Network.h"
#include "core/State.h"
#include <Arduino.h>
#include <stdio.h>

static void render(lv_obj_t* root) {
    int batPct   = hal::readBatteryPercent();
    float batV   = hal::readBatteryVoltage();
    bool online  = state::isOnline();
    int rssi     = svc::wifiRssi();

    // Battery Gauge
    char batBuf[16];
    snprintf(batBuf, sizeof(batBuf), "%d%%", batPct);
    ui::gauge(root, 10, 57, 98, batPct, 0, 100, batBuf, "Battery",
              (batPct <= 20) ? CLR_RED : CLR_GREEN);

    // WiFi RSSI Gauge
    int signalVal = online ? constrain(rssi + 100, 0, 70) : 0;
    char rssiBuf[16];
    if (online) {
        snprintf(rssiBuf, sizeof(rssiBuf), "%d", rssi);
    } else {
        snprintf(rssiBuf, sizeof(rssiBuf), "--");
    }
    ui::gauge(root, 132, 57, 98, signalVal, 0, 70, rssiBuf, "WiFi dBm",
              online ? CLR_CYAN : CLR_DIM);

    // HEAP Card
    char heapBuf[16];
    snprintf(heapBuf, sizeof(heapBuf), "%u KB", ESP.getFreeHeap() / 1024);
    ui::metricCard(root, 10, 172, layout::CARD_W_HALF, 54, "HEAP", heapBuf, "Free", CLR_CYAN);

    // PSRAM Card
    char psramBuf[16];
    const char* psramCap = "Not found";
    if (psramFound()) {
        snprintf(psramBuf, sizeof(psramBuf), "%u KB", ESP.getFreePsram() / 1024);
        psramCap = "Free";
    } else {
        snprintf(psramBuf, sizeof(psramBuf), "--");
    }
    ui::metricCard(root, 125, 172, layout::CARD_W_HALF, 54, "PSRAM", psramBuf, psramCap, CLR_PURPLE);

    // STATUS Card
    char statusCapBuf[64];
    snprintf(statusCapBuf, sizeof(statusCapBuf), "%.2f V   SD %s",
             batV, state::isSdReady() ? "ready" : "offline");
    ui::metricCard(root, 10, 237, layout::CARD_W_FULL, 52, "STATUS",
                   online ? "ONLINE" : "OFFLINE", statusCapBuf,
                   online ? CLR_GREEN : CLR_RED);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_SYSTEM,
    { "SYSTEM", CLR_SYSTEM_TOP, render }
);
