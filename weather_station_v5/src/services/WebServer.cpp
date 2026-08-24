#include "src/services/WebServer.h"
#include "src/services/Storage.h"
#include "src/services/Api.h"
#include "src/hal/Backlight.h"
#include "src/hal/Battery.h"
#include "src/core/State.h"
#include "src/Config.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoJson.h>

namespace svc {

static WebServer s_server(80);

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
  <title>MeteorS3 &bull; Weather Station</title>
  <style>
    :root {
      --bg: #090b0e;
      --card-bg: #12151c;
      --card-inner: #181c26;
      --border: rgba(255, 255, 255, 0.08);
      --border-focus: #3b82f6;
      --text: #f1f5f9;
      --text-muted: #8896a6;
      --text-dim: #546274;
      --accent: #3b82f6;
      --accent-hover: #2563eb;
      --emerald: #10b981;
      --amber: #f59e0b;
      --rose: #f43f5e;
      --radius-sm: 8px;
      --radius-md: 12px;
      --radius-lg: 16px;
      --shadow: 0 4px 20px -2px rgba(0, 0, 0, 0.5);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background: var(--bg);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      padding: 32px 16px 48px;
      -webkit-font-smoothing: antialiased;
    }
    .wrapper { width: 100%; max-width: 840px; }

    /* Header */
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 24px;
      padding-bottom: 20px;
      border-bottom: 1px solid var(--border);
    }
    .brand { display: flex; align-items: center; gap: 14px; }
    .brand-logo {
      width: 40px; height: 40px;
      background: var(--card-inner);
      border: 1px solid var(--border);
      border-radius: var(--radius-md);
      display: flex; align-items: center; justify-content: center;
      color: var(--accent);
    }
    .brand-logo svg { width: 22px; height: 22px; }
    .brand-meta h1 { font-size: 18px; font-weight: 600; letter-spacing: -0.3px; color: var(--text); }
    .brand-meta p { font-size: 12px; color: var(--text-muted); margin-top: 2px; }

    /* Status Badge */
    .status-pill {
      display: inline-flex;
      align-items: center;
      gap: 7px;
      padding: 6px 14px;
      border-radius: 999px;
      font-size: 12px;
      font-weight: 500;
      background: var(--card-bg);
      border: 1px solid var(--border);
      color: var(--text-muted);
    }
    .dot {
      width: 8px; height: 8px;
      border-radius: 50%;
      background: var(--text-dim);
    }
    .status-pill.online { color: var(--emerald); border-color: rgba(16, 185, 129, 0.25); }
    .status-pill.online .dot { background: var(--emerald); box-shadow: 0 0 8px rgba(16, 185, 129, 0.6); }
    .status-pill.ap { color: var(--amber); border-color: rgba(245, 158, 11, 0.25); }
    .status-pill.ap .dot { background: var(--amber); box-shadow: 0 0 8px rgba(245, 158, 11, 0.6); }
    .status-pill.offline { color: var(--rose); border-color: rgba(244, 63, 94, 0.25); }
    .status-pill.offline .dot { background: var(--rose); }

    /* Navigation Bar */
    .nav-bar {
      display: flex;
      gap: 6px;
      background: var(--card-bg);
      padding: 5px;
      border-radius: var(--radius-md);
      border: 1px solid var(--border);
      margin-bottom: 24px;
      overflow-x: auto;
      scrollbar-width: none;
    }
    .nav-bar::-webkit-scrollbar { display: none; }
    .nav-item {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      background: transparent;
      border: none;
      color: var(--text-muted);
      padding: 9px 16px;
      border-radius: var(--radius-sm);
      cursor: pointer;
      font-size: 13px;
      font-weight: 500;
      white-space: nowrap;
      transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
    }
    .nav-item svg { width: 16px; height: 16px; opacity: 0.7; }
    .nav-item:hover { color: var(--text); background: rgba(255,255,255,0.03); }
    .nav-item.active {
      background: var(--card-inner);
      color: var(--text);
      box-shadow: 0 1px 3px rgba(0,0,0,0.3);
      border: 1px solid var(--border);
    }
    .nav-item.active svg { opacity: 1; color: var(--accent); }

    /* Tab Panes */
    .tab-content { display: none; }
    .tab-content.active { display: block; animation: fadeSlide 0.25s ease-out; }
    @keyframes fadeSlide {
      from { opacity: 0; transform: translateY(4px); }
      to { opacity: 1; transform: translateY(0); }
    }

    /* Cards & Grids */
    .card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: var(--radius-lg);
      padding: 24px;
      margin-bottom: 20px;
      box-shadow: var(--shadow);
    }
    .card-header {
      margin-bottom: 20px;
      padding-bottom: 14px;
      border-bottom: 1px solid var(--border);
    }
    .card-title { font-size: 15px; font-weight: 600; color: var(--text); }
    .card-subtitle { font-size: 12px; color: var(--text-muted); margin-top: 3px; }

    .grid-metrics {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 14px;
      margin-bottom: 20px;
    }
    @media (max-width: 720px) {
      .grid-metrics { grid-template-columns: repeat(2, 1fr); }
    }

    .metric-box {
      background: var(--card-inner);
      border: 1px solid var(--border);
      border-radius: var(--radius-md);
      padding: 16px;
    }
    .metric-label {
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: 0.05em;
      color: var(--text-dim);
      font-weight: 600;
      margin-bottom: 8px;
    }
    .metric-value {
      font-size: 24px;
      font-weight: 600;
      color: var(--text);
      font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace;
      letter-spacing: -0.5px;
    }
    .metric-sub {
      font-size: 12px;
      color: var(--text-muted);
      margin-top: 4px;
    }

    .grid-2 {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 20px;
    }
    @media (max-width: 720px) {
      .grid-2 { grid-template-columns: 1fr; }
    }

    /* Key-Value Telemetry Rows */
    .telem-list { display: flex; flex-direction: column; gap: 10px; margin-bottom: 18px; }
    .telem-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 13px;
      padding-bottom: 8px;
      border-bottom: 1px solid rgba(255,255,255,0.04);
    }
    .telem-key { color: var(--text-muted); }
    .telem-val { font-weight: 500; color: var(--text); font-family: ui-monospace, monospace; }

    /* Forms */
    .form-group { margin-bottom: 18px; }
    .form-group:last-child { margin-bottom: 0; }
    label {
      display: block;
      font-size: 12px;
      font-weight: 500;
      color: var(--text-muted);
      margin-bottom: 6px;
      letter-spacing: 0.01em;
    }
    .field-hint { font-size: 11px; color: var(--text-dim); margin-top: 4px; }

    input[type="text"],
    input[type="password"],
    input[type="number"],
    select {
      width: 100%;
      background: var(--card-inner);
      border: 1px solid var(--border);
      border-radius: var(--radius-sm);
      padding: 10px 14px;
      color: var(--text);
      font-size: 13px;
      font-family: inherit;
      outline: none;
      transition: border-color 0.2s, box-shadow 0.2s;
    }
    input:focus, select:focus {
      border-color: var(--border-focus);
      box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.15);
    }
    select {
      appearance: none;
      background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' fill='%238896a6' viewBox='0 0 16 16'%3E%3Cpath d='M8 11.5l-5-5h10l-5 5z'/%3E%3C/svg%3E");
      background-repeat: no-repeat;
      background-position: right 14px center;
      padding-right: 32px;
    }

    .form-row { display: flex; gap: 14px; }
    .form-col { flex: 1; }

    /* Range Slider */
    .slider-wrap { display: flex; align-items: center; gap: 14px; }
    input[type="range"] {
      flex: 1;
      accent-color: var(--accent);
      background: var(--card-inner);
      height: 6px;
      border-radius: 3px;
      outline: none;
    }
    .slider-badge {
      background: var(--card-inner);
      border: 1px solid var(--border);
      padding: 4px 10px;
      border-radius: var(--radius-sm);
      font-size: 12px;
      font-weight: 500;
      min-width: 44px;
      text-align: center;
      font-family: ui-monospace, monospace;
    }

    /* Sleek Switch Toggle */
    .switch-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 10px 0;
    }
    .switch-label { font-size: 13px; font-weight: 500; color: var(--text); }
    .switch-desc { font-size: 12px; color: var(--text-dim); margin-top: 2px; }
    .toggle {
      position: relative;
      display: inline-block;
      width: 42px; height: 24px;
    }
    .toggle input { opacity: 0; width: 0; height: 0; }
    .toggle-slider {
      position: absolute; cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background: var(--card-inner);
      border: 1px solid var(--border);
      border-radius: 24px;
      transition: all 0.25s;
    }
    .toggle-slider:before {
      position: absolute; content: "";
      height: 16px; width: 16px;
      left: 3px; bottom: 3px;
      background: var(--text-muted);
      border-radius: 50%;
      transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
    }
    .toggle input:checked + .toggle-slider {
      background: var(--accent);
      border-color: var(--accent);
    }
    .toggle input:checked + .toggle-slider:before {
      transform: translateX(18px);
      background: #ffffff;
    }

    /* Buttons */
    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      background: var(--accent);
      color: #ffffff;
      border: none;
      padding: 10px 20px;
      border-radius: var(--radius-sm);
      font-size: 13px;
      font-weight: 500;
      font-family: inherit;
      cursor: pointer;
      transition: all 0.2s;
    }
    .btn:hover { background: var(--accent-hover); }
    .btn:active { transform: scale(0.98); }
    .btn-secondary {
      background: var(--card-inner);
      color: var(--text);
      border: 1px solid var(--border);
    }
    .btn-secondary:hover { background: rgba(255,255,255,0.06); border-color: rgba(255,255,255,0.15); }
    .btn-danger {
      background: rgba(244, 63, 94, 0.15);
      color: var(--rose);
      border: 1px solid rgba(244, 63, 94, 0.25);
    }
    .btn-danger:hover { background: rgba(244, 63, 94, 0.25); }
    .btn-group { display: flex; gap: 10px; margin-top: 20px; }

    /* OTA Dropzone */
    .file-upload-box {
      border: 1px dashed var(--border);
      border-radius: var(--radius-md);
      padding: 28px 16px;
      text-align: center;
      background: var(--card-inner);
      margin-bottom: 18px;
    }
    .file-upload-box input[type="file"] { display: none; }
    .file-label {
      cursor: pointer;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 10px;
      color: var(--text-muted);
    }
    .file-label svg { width: 28px; height: 28px; opacity: 0.6; }
    .file-name-preview { font-size: 12px; color: var(--accent); margin-top: 6px; font-weight: 500; }

    /* Toast Notification */
    .toast {
      position: fixed;
      bottom: 24px;
      right: 24px;
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: var(--radius-md);
      padding: 12px 20px;
      font-size: 13px;
      color: var(--text);
      box-shadow: var(--shadow);
      display: none;
      z-index: 100;
      animation: fadeSlide 0.2s ease-out;
    }
    .toast.show { display: flex; align-items: center; gap: 8px; }
  </style>
</head>
<body>
  <div class="wrapper">
    <!-- Header -->
    <header>
      <div class="brand">
        <div class="brand-logo">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z"/>
          </svg>
        </div>
        <div class="brand-meta">
          <h1>MeteorS3 Hub</h1>
          <p>ESP32-S3 Weather Station Console</p>
        </div>
      </div>
      <div id="connBadge" class="status-pill">
        <span class="dot"></span>
        <span id="connText">Initializing...</span>
      </div>
    </header>

    <!-- Navigation -->
    <nav class="nav-bar">
      <button class="nav-item active" onclick="showTab(event, 'dashboard')">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect width="7" height="9" x="3" y="3" rx="1"/><rect width="7" height="5" x="14" y="3" rx="1"/><rect width="7" height="9" x="14" y="12" rx="1"/><rect width="7" height="5" x="3" y="16" rx="1"/></svg>
        Dashboard
      </button>
      <button class="nav-item" onclick="showTab(event, 'wifi')">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><line x1="12" y1="20" x2="12.01" y2="20"/></svg>
        Network
      </button>
      <button class="nav-item" onclick="showTab(event, 'location')">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 10c0 6-8 12-8 12s-8-6-8-12a8 8 0 0 1 16 0Z"/><circle cx="12" cy="10" r="3"/></svg>
        Location
      </button>
      <button class="nav-item" onclick="showTab(event, 'display')">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect width="20" height="14" x="2" y="3" rx="2"/><line x1="8" y1="21" x2="16" y2="21"/><line x1="12" y1="17" x2="12" y2="21"/></svg>
        Display
      </button>
      <button class="nav-item" onclick="showTab(event, 'mqtt')">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 11a9 9 0 0 1 9 9"/><path d="M4 4a16 16 0 0 1 16 16"/><circle cx="5" cy="19" r="1"/></svg>
        MQTT
      </button>
      <button class="nav-item" onclick="showTab(event, 'system')">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/></svg>
        Firmware
      </button>
    </nav>

    <!-- TAB 1: DASHBOARD -->
    <section id="tab-dashboard" class="tab-content active">
      <div class="grid-metrics">
        <div class="metric-box">
          <div class="metric-label">Temperature</div>
          <div class="metric-value" id="valTemp">-- &deg;C</div>
          <div class="metric-sub" id="valFeels">Feels --</div>
        </div>
        <div class="metric-box">
          <div class="metric-label">Air Quality</div>
          <div class="metric-value" id="valAqi">--</div>
          <div class="metric-sub" id="valAqiCat">AQI Index</div>
        </div>
        <div class="metric-box">
          <div class="metric-label">Humidity</div>
          <div class="metric-value" id="valHumidity">--%</div>
          <div class="metric-sub" id="valDew">Relative</div>
        </div>
        <div class="metric-box">
          <div class="metric-label">Pressure</div>
          <div class="metric-value" id="valPressure">--</div>
          <div class="metric-sub" id="valTrend">Barometer</div>
        </div>
      </div>

      <div class="grid-2">
        <div class="card">
          <div class="card-header">
            <div class="card-title">Device Telemetry</div>
            <div class="card-subtitle">Hardware statistics and power metrics</div>
          </div>
          <div class="telem-list">
            <div class="telem-row">
              <span class="telem-key">Battery Status</span>
              <span class="telem-val"><span id="valBat">--%</span> &bull; <span id="valBatV">-- V</span></span>
            </div>
            <div class="telem-row">
              <span class="telem-key">WiFi RSSI</span>
              <span class="telem-val" id="valRssi">-- dBm</span>
            </div>
            <div class="telem-row">
              <span class="telem-key">Heap / PSRAM</span>
              <span class="telem-val"><span id="valHeap">-- KB</span> / <span id="valPsram">-- KB</span></span>
            </div>
            <div class="telem-row">
              <span class="telem-key">System Uptime</span>
              <span class="telem-val" id="valUptime">--</span>
            </div>
          </div>
          <div class="btn-group">
            <button class="btn btn-secondary" onclick="triggerRefresh()">Sync Weather</button>
            <button class="btn btn-danger" onclick="rebootDevice()">Restart Hub</button>
          </div>
        </div>

        <div class="card">
          <div class="card-header">
            <div class="card-title">Weather Intelligence</div>
            <div class="card-subtitle">Automated meteorological summaries & alerts</div>
          </div>
          <p id="valInsightPrimary" style="font-size: 14px; font-weight: 500; color: var(--text); margin-bottom: 8px;">Awaiting weather data...</p>
          <p id="valInsightSec" style="font-size: 13px; color: var(--text-muted); line-height: 1.5; margin-bottom: 16px;">--</p>
          <div style="padding-top: 12px; border-top: 1px solid var(--border);">
            <div class="metric-label" style="margin-bottom: 6px;">Active Bulletins</div>
            <div id="alertsList" style="font-size: 13px; color: var(--amber);">All clear</div>
          </div>
        </div>
      </div>
    </section>

    <!-- TAB 2: NETWORK -->
    <section id="tab-wifi" class="tab-content">
      <div class="card">
        <div class="card-header">
          <div class="card-title">Wireless Network</div>
          <div class="card-subtitle">Connect MeteorS3 to your 2.4 GHz WiFi router</div>
        </div>
        <form id="wifiForm" onsubmit="saveConfig(event)">
          <div class="form-group">
            <label>Network SSID</label>
            <input type="text" id="cfgSsid" name="ssid" required placeholder="WiFi network name">
            <div class="field-hint">Must be a 2.4 GHz wireless network (802.11 b/g/n).</div>
          </div>
          <div class="form-group">
            <label>Security Key</label>
            <input type="password" id="cfgPass" name="pass" placeholder="Network password">
          </div>
          <div class="btn-group">
            <button type="submit" class="btn">Apply & Connect</button>
          </div>
        </form>
      </div>
    </section>

    <!-- TAB 3: LOCATION -->
    <section id="tab-location" class="tab-content">
      <div class="card">
        <div class="card-header">
          <div class="card-title">Location & Regional Units</div>
          <div class="card-subtitle">Geographic coordinates for Open-Meteo forecast queries</div>
        </div>
        <form id="locForm" onsubmit="saveConfig(event)">
          <div class="form-group">
            <label>Station Location Moniker</label>
            <input type="text" id="cfgLoc" name="loc" required placeholder="e.g. HOME, TOKYO, BALI">
          </div>
          <div class="form-row">
            <div class="form-col form-group">
              <label>Latitude</label>
              <input type="number" step="0.000001" id="cfgLat" name="lat" required>
            </div>
            <div class="form-col form-group">
              <label>Longitude</label>
              <input type="number" step="0.000001" id="cfgLon" name="lon" required>
            </div>
          </div>
          <div class="form-group">
            <label>IANA Timezone</label>
            <input type="text" id="cfgTz" name="tz" required placeholder="e.g. Asia/Kolkata, Europe/London">
          </div>
          <div class="form-row">
            <div class="form-col form-group">
              <label>Temperature Unit</label>
              <select id="cfgTu" name="tu">
                <option value="0">Celsius (&deg;C)</option>
                <option value="1">Fahrenheit (&deg;F)</option>
              </select>
            </div>
            <div class="form-col form-group">
              <label>Wind Unit</label>
              <select id="cfgWu" name="wu">
                <option value="0">km/h</option>
                <option value="1">mph</option>
                <option value="2">m/s</option>
              </select>
            </div>
            <div class="form-col form-group">
              <label>Pressure Unit</label>
              <select id="cfgPu" name="pu">
                <option value="0">hPa</option>
                <option value="1">inHg</option>
              </select>
            </div>
          </div>
          <div class="btn-group">
            <button type="submit" class="btn">Save Location & Units</button>
          </div>
        </form>
      </div>
    </section>

    <!-- TAB 4: DISPLAY -->
    <section id="tab-display" class="tab-content">
      <div class="card">
        <div class="card-header">
          <div class="card-title">Display & Motion Controls</div>
          <div class="card-subtitle">Backlight brightness, sleep timers, and IMU gestures</div>
        </div>
        <form id="dispForm" onsubmit="saveConfig(event)">
          <div class="form-group">
            <label>Backlight Level</label>
            <div class="slider-wrap">
              <input type="range" min="10" max="255" id="cfgBl" name="bl" oninput="document.getElementById('lblBright').innerText = this.value">
              <span class="slider-badge" id="lblBright">220</span>
            </div>
          </div>
          <div class="form-group">
            <label>Screen Timeout (seconds, 0 = always on)</label>
            <input type="number" min="0" max="3600" id="cfgTout" name="tout">
          </div>
          <div class="switch-row">
            <div>
              <div class="switch-label">IMU Motion Gestures</div>
              <div class="switch-desc">Tilt left/right to browse pages, shake to return home</div>
            </div>
            <label class="toggle">
              <input type="checkbox" id="cfgGest" name="gest">
              <span class="toggle-slider"></span>
            </label>
          </div>
          <div class="form-group" style="margin-top: 12px;">
            <label>Gesture Sensitivity</label>
            <div class="slider-wrap">
              <input type="range" min="1" max="10" id="cfgGsens" name="gsens" oninput="document.getElementById('lblSens').innerText = this.value">
              <span class="slider-badge" id="lblSens">5</span>
            </div>
          </div>
          <div class="switch-row" style="margin-top: 8px;">
            <div>
              <div class="switch-label">Auto-Rotate Pages</div>
              <div class="switch-desc">Cycle through dashboard screens automatically</div>
            </div>
            <label class="toggle">
              <input type="checkbox" id="cfgApg" name="apg">
              <span class="toggle-slider"></span>
            </label>
          </div>
          <div class="form-group" style="margin-top: 12px;">
            <label>Auto-Rotate Interval (seconds)</label>
            <input type="number" min="5" max="300" id="cfgApgs" name="apgs">
          </div>
          <div class="btn-group">
            <button type="submit" class="btn">Save Display Settings</button>
          </div>
        </form>
      </div>
    </section>

    <!-- TAB 5: MQTT -->
    <section id="tab-mqtt" class="tab-content">
      <div class="card">
        <div class="card-header">
          <div class="card-title">Home Assistant & MQTT</div>
          <div class="card-subtitle">Stream sensor telemetry to MQTT broker / Home Assistant</div>
        </div>
        <form id="mqttForm" onsubmit="saveConfig(event)">
          <div class="switch-row">
            <div>
              <div class="switch-label">Enable MQTT Client</div>
              <div class="switch-desc">Publishes live telemetry and Home Assistant auto-discovery</div>
            </div>
            <label class="toggle">
              <input type="checkbox" id="cfgMqEn" name="mq_en">
              <span class="toggle-slider"></span>
            </label>
          </div>
          <div class="form-row" style="margin-top: 14px;">
            <div class="form-col form-group" style="flex: 2;">
              <label>Broker Host / IP</label>
              <input type="text" id="cfgMqSrv" name="mq_srv" placeholder="192.168.1.100 or homeassistant.local">
            </div>
            <div class="form-col form-group" style="flex: 1;">
              <label>Port</label>
              <input type="number" id="cfgMqPort" name="mq_port" value="1883">
            </div>
          </div>
          <div class="form-row">
            <div class="form-col form-group">
              <label>Username (Optional)</label>
              <input type="text" id="cfgMqUsr" name="mq_usr">
            </div>
            <div class="form-col form-group">
              <label>Password (Optional)</label>
              <input type="password" id="cfgMqPwd" name="mq_pwd">
            </div>
          </div>
          <div class="form-group">
            <label>Topic Prefix</label>
            <input type="text" id="cfgMqPfx" name="mq_pfx" value="weatherstation">
          </div>
          <div class="btn-group">
            <button type="submit" class="btn">Save MQTT Settings</button>
          </div>
        </form>
      </div>
    </section>

    <!-- TAB 6: FIRMWARE -->
    <section id="tab-system" class="tab-content">
      <div class="card">
        <div class="card-header">
          <div class="card-title">Over-The-Air Firmware Update</div>
          <div class="card-subtitle">Flash updated firmware binary directly to the ESP32-S3</div>
        </div>
        <form method="POST" action="/update" enctype="multipart/form-data">
          <div class="file-upload-box">
            <label class="file-label" for="otaFile">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/></svg>
              <span>Click to choose compiled <strong>.bin</strong> binary</span>
              <input type="file" id="otaFile" name="update" accept=".bin" required onchange="document.getElementById('otaFileName').innerText = this.files[0] ? this.files[0].name : ''">
            </label>
            <div id="otaFileName" class="file-name-preview"></div>
          </div>
          <button type="submit" class="btn">Upload & Flash Firmware</button>
        </form>
      </div>
    </section>
  </div>

  <div id="toast" class="toast">Settings applied successfully</div>

  <script>
    function showTab(e, id) {
      document.querySelectorAll('.nav-item').forEach(b => b.classList.remove('active'));
      document.querySelectorAll('.tab-content').forEach(p => p.classList.remove('active'));
      e.currentTarget.classList.add('active');
      document.getElementById('tab-' + id).classList.add('active');
    }

    function notify(msg) {
      const t = document.getElementById('toast');
      t.innerText = msg;
      t.classList.add('show');
      setTimeout(() => t.classList.remove('show'), 3000);
    }

    async function loadStatus() {
      try {
        const res = await fetch('/api/status');
        const d = await res.json();

        const badge = document.getElementById('connBadge');
        const text = document.getElementById('connText');
        badge.className = 'status-pill ' + (d.online ? 'online' : (d.apMode ? 'ap' : 'offline'));
        text.innerText = d.online ? ('Online &bull; ' + d.ip) : (d.apMode ? 'AP Setup Mode' : 'Offline');

        document.getElementById('valTemp').innerHTML = (d.temp !== null ? d.temp.toFixed(1) : '--') + ' &deg;C';
        document.getElementById('valFeels').innerText = 'Feels ' + (d.apparent !== null ? d.apparent.toFixed(1) : '--') + ' \u00B0C';
        document.getElementById('valAqi').innerText = d.aqi >= 0 ? d.aqi : '--';
        document.getElementById('valHumidity').innerText = (d.humidity !== null ? d.humidity.toFixed(0) : '--') + '%';
        document.getElementById('valPressure').innerText = (d.pressure !== null ? d.pressure.toFixed(1) : '--') + ' hPa';

        document.getElementById('valBat').innerText = d.battery + '%';
        document.getElementById('valBatV').innerText = d.batteryVoltage.toFixed(2) + ' V';
        document.getElementById('valRssi').innerText = d.rssi + ' dBm';
        document.getElementById('valHeap').innerText = Math.round(d.freeHeap / 1024) + ' KB';
        document.getElementById('valPsram').innerText = Math.round(d.freePsram / 1024) + ' KB';
        document.getElementById('valUptime').innerText = Math.floor(d.uptime / 60) + ' min';

        document.getElementById('valInsightPrimary').innerText = d.insightPrimary || 'Normal conditions';
        document.getElementById('valInsightSec').innerText = d.insightSec || '';

        if (d.alerts && d.alerts.length > 0) {
          document.getElementById('alertsList').innerHTML = d.alerts.map(a => `<p>&bull; <strong>${a.title}</strong>: ${a.detail}</p>`).join('');
        } else {
          document.getElementById('alertsList').innerText = 'All clear';
        }
      } catch(e) { console.error(e); }
    }

    async function loadConfig() {
      try {
        const res = await fetch('/api/config');
        const c = await res.json();

        document.getElementById('cfgSsid').value = c.ssid || '';
        document.getElementById('cfgLoc').value = c.loc || 'HOME';
        document.getElementById('cfgLat').value = c.lat || 30.0;
        document.getElementById('cfgLon').value = c.lon || 75.0;
        document.getElementById('cfgTz').value = c.tz || 'Asia/Kolkata';
        document.getElementById('cfgTu').value = c.tu || 0;
        document.getElementById('cfgWu').value = c.wu || 0;
        document.getElementById('cfgPu').value = c.pu || 0;

        document.getElementById('cfgBl').value = c.bl || 220;
        document.getElementById('lblBright').innerText = c.bl || 220;
        document.getElementById('cfgTout').value = c.tout || 60;
        document.getElementById('cfgGest').checked = !!c.gest;
        document.getElementById('cfgGsens').value = c.gsens || 5;
        document.getElementById('lblSens').innerText = c.gsens || 5;
        document.getElementById('cfgApg').checked = !!c.apg;
        document.getElementById('cfgApgs').value = c.apgs || 30;

        document.getElementById('cfgMqEn').checked = !!c.mq_en;
        document.getElementById('cfgMqSrv').value = c.mq_srv || '';
        document.getElementById('cfgMqPort').value = c.mq_port || 1883;
        document.getElementById('cfgMqUsr').value = c.mq_usr || '';
        document.getElementById('cfgMqPfx').value = c.mq_pfx || 'weatherstation';
      } catch(e) { console.error(e); }
    }

    async function saveConfig(e) {
      e.preventDefault();
      const form = e.target;
      const data = {};
      new FormData(form).forEach((v, k) => { data[k] = v; });

      if (form.id === 'dispForm') {
        data.gest = document.getElementById('cfgGest').checked;
        data.apg = document.getElementById('cfgApg').checked;
      }
      if (form.id === 'mqttForm') {
        data.mq_en = document.getElementById('cfgMqEn').checked;
      }

      try {
        const res = await fetch('/api/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(data)
        });
        if (res.ok) {
          notify('Settings saved & applied');
          loadConfig();
        }
      } catch(err) { alert('Failed to save settings: ' + err); }
    }

    async function triggerRefresh() {
      await fetch('/api/refresh', { method: 'POST' });
      notify('Sync request sent');
      setTimeout(loadStatus, 1500);
    }

    async function rebootDevice() {
      if (confirm('Restart MeteorS3 Hub?')) {
        await fetch('/api/reboot', { method: 'POST' });
        notify('Restarting device...');
      }
    }

    loadStatus();
    loadConfig();
    setInterval(loadStatus, 5000);
  </script>
</body>
</html>
)rawliteral";

static void handleRoot() {
    s_server.send_P(200, "text/html", INDEX_HTML);
}

static void handleApiStatus() {
    JsonDocument doc;

    state::lock();
    const WeatherData& w     = state::weather();
    const AirData&     a     = state::air();
    const InsightData& in    = state::insights();
    const AlertItem*   al    = state::alerts();
    int                alCnt = state::alertCount();
    bool               onl   = state::isOnline();
    bool               ap    = state::isApMode();
    SystemTelemetry    telem = state::telemetry();
    state::unlock();

    doc["online"]        = onl;
    doc["apMode"]        = ap;
    doc["ip"]            = telem.ipAddress;
    doc["battery"]       = hal::readBatteryPercent();
    doc["batteryVoltage"]= hal::readBatteryVoltage();
    doc["rssi"]          = telem.wifiRssi;
    doc["freeHeap"]      = ESP.getFreeHeap();
    doc["freePsram"]     = ESP.getFreePsram();
    doc["uptime"]        = millis() / 1000UL;

    if (w.valid) {
        doc["temp"]      = w.temperature;
        doc["apparent"]  = w.apparent;
        doc["humidity"]  = w.humidity;
        doc["pressure"]  = w.pressure;
        doc["wind"]      = w.wind;
        doc["weatherCode"] = w.weatherCode;
    } else {
        doc["temp"]      = nullptr;
        doc["apparent"]  = nullptr;
        doc["humidity"]  = nullptr;
        doc["pressure"]  = nullptr;
    }

    doc["aqi"]           = a.valid ? a.usAqi : -1;
    doc["insightPrimary"]= in.primary;
    doc["insightSec"]    = in.secondary;

    JsonArray arr = doc["alerts"].to<JsonArray>();
    for (int i = 0; i < min(5, alCnt); i++) {
        if (al[i].title[0] != '\0') {
            JsonObject obj = arr.add<JsonObject>();
            obj["title"]  = al[i].title;
            obj["detail"] = al[i].detail;
        }
    }

    String json;
    serializeJson(doc, json);
    s_server.send(200, "application/json", json);
}

static void handleApiConfigGet() {
    JsonDocument doc;
    state::lock();
    const RuntimeConfig& c = state::config();

    doc["ssid"]    = c.wifiSsid;
    doc["loc"]     = c.locationName;
    doc["lat"]     = c.latitude;
    doc["lon"]     = c.longitude;
    doc["tz"]      = c.timezone;
    doc["tu"]      = (int)c.tempUnit;
    doc["wu"]      = (int)c.windUnit;
    doc["pu"]      = (int)c.pressUnit;

    doc["bl"]      = c.backlightBrightness;
    doc["tout"]    = c.screenTimeoutSec;
    doc["gest"]    = c.enableGestures;
    doc["gsens"]   = c.gestureSensitivity;
    doc["apg"]     = c.enableAutoPage;
    doc["apgs"]    = c.autoPageSec;

    doc["mq_en"]   = c.mqttEnabled;
    doc["mq_srv"]  = c.mqttServer;
    doc["mq_port"] = c.mqttPort;
    doc["mq_usr"]  = c.mqttUser;
    doc["mq_pfx"]  = c.mqttTopicPrefix;
    state::unlock();

    String json;
    serializeJson(doc, json);
    s_server.send(200, "application/json", json);
}

static void handleApiConfigPost() {
    if (!s_server.hasArg("plain")) {
        s_server.send(400, "text/plain", "Missing body");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, s_server.arg("plain"));
    if (err) {
        s_server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    state::lock();
    RuntimeConfig c = state::config();
    state::unlock();

    if (doc["ssid"].is<const char*>()) snprintf(c.wifiSsid, sizeof(c.wifiSsid), "%s", doc["ssid"].as<const char*>());
    if (doc["pass"].is<const char*>()) snprintf(c.wifiPassword, sizeof(c.wifiPassword), "%s", doc["pass"].as<const char*>());
    if (doc["loc"].is<const char*>())  snprintf(c.locationName, sizeof(c.locationName), "%s", doc["loc"].as<const char*>());
    if (doc["lat"].is<double>())       c.latitude = doc["lat"].as<double>();
    if (doc["lon"].is<double>())       c.longitude = doc["lon"].as<double>();
    if (doc["tz"].is<const char*>())   snprintf(c.timezone, sizeof(c.timezone), "%s", doc["tz"].as<const char*>());

    if (doc["tu"].is<int>())           c.tempUnit = (TempUnit)doc["tu"].as<int>();
    if (doc["wu"].is<int>())           c.windUnit = (WindUnit)doc["wu"].as<int>();
    if (doc["pu"].is<int>())           c.pressUnit = (PressUnit)doc["pu"].as<int>();

    if (doc["bl"].is<int>()) {
        c.backlightBrightness = doc["bl"].as<int>();
        hal::backlightSetBrightness(c.backlightBrightness, true);
    }
    if (doc["tout"].is<uint32_t>())    c.screenTimeoutSec = doc["tout"].as<uint32_t>();
    if (doc["gest"].is<bool>())        c.enableGestures = doc["gest"].as<bool>();
    if (doc["gsens"].is<int>())        c.gestureSensitivity = doc["gsens"].as<int>();
    if (doc["apg"].is<bool>())         c.enableAutoPage = doc["apg"].as<bool>();
    if (doc["apgs"].is<uint32_t>())    c.autoPageSec = doc["apgs"].as<uint32_t>();

    if (doc["mq_en"].is<bool>())       c.mqttEnabled = doc["mq_en"].as<bool>();
    if (doc["mq_srv"].is<const char*>()) snprintf(c.mqttServer, sizeof(c.mqttServer), "%s", doc["mq_srv"].as<const char*>());
    if (doc["mq_port"].is<uint16_t>()) c.mqttPort = doc["mq_port"].as<uint16_t>();
    if (doc["mq_usr"].is<const char*>()) snprintf(c.mqttUser, sizeof(c.mqttUser), "%s", doc["mq_usr"].as<const char*>());
    if (doc["mq_pwd"].is<const char*>()) snprintf(c.mqttPassword, sizeof(c.mqttPassword), "%s", doc["mq_pwd"].as<const char*>());
    if (doc["mq_pfx"].is<const char*>()) snprintf(c.mqttTopicPrefix, sizeof(c.mqttTopicPrefix), "%s", doc["mq_pfx"].as<const char*>());

    storageSaveConfig(c);
    s_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

static void handleApiRefresh() {
    apiForceRefresh();
    s_server.send(200, "application/json", "{\"status\":\"refreshed\"}");
}

static void handleApiReboot() {
    s_server.send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(500);
    ESP.restart();
}

static void handleCaptivePortal() {
    s_server.sendHeader("Location", "/", true);
    s_server.send(302, "text/plain", "");
}

static void handleOtaUpload() {
    HTTPUpload& upload = s_server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[ota] starting update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("[ota] success! %u bytes. Rebooting...\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    }
}

void webServerInit() {
    s_server.on("/", HTTP_GET, handleRoot);
    s_server.on("/api/status", HTTP_GET, handleApiStatus);
    s_server.on("/api/config", HTTP_GET, handleApiConfigGet);
    s_server.on("/api/config", HTTP_POST, handleApiConfigPost);
    s_server.on("/api/refresh", HTTP_POST, handleApiRefresh);
    s_server.on("/api/reboot", HTTP_POST, handleApiReboot);

    // Captive portal probes
    s_server.on("/generate_204", handleCaptivePortal);
    s_server.on("/hotspot-detect.html", handleCaptivePortal);
    s_server.on("/ncsi.txt", handleCaptivePortal);

    // Web OTA update endpoint
    s_server.on("/update", HTTP_POST, []() {
        s_server.send(200, "text/html", "<h2>Update " + String(Update.hasError() ? "Failed" : "Succeeded! Rebooting...") + "</h2>");
        delay(1000);
        ESP.restart();
    }, handleOtaUpload);

    s_server.onNotFound(handleCaptivePortal);
    s_server.begin();
    Serial.println("[web] HTTP server listening on port 80");
}

void webServerService() {
    s_server.handleClient();
}

}  // namespace svc
