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
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>MeteorS3 — Weather Station Hub</title>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&family=JetBrains+Mono:wght@400;600&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #070e17;
      --card: rgba(18, 32, 48, 0.7);
      --card-border: rgba(104, 218, 234, 0.15);
      --card-hover: rgba(26, 44, 66, 0.85);
      --cyan: #68daea;
      --blue: #78afff;
      --green: #75dba0;
      --yellow: #ffd57a;
      --orange: #ffaa67;
      --red: #ff7f83;
      --purple: #b9a3ff;
      --text: #f5f8fa;
      --muted: #94a5ae;
      --dim: #5d707a;
      --glow: 0 0 20px rgba(104, 218, 234, 0.25);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Outfit', sans-serif;
      background: radial-gradient(circle at 50% 0%, #0d283c 0%, var(--bg) 75%);
      color: var(--text);
      min-height: 100vh;
      padding: 24px 16px;
      display: flex;
      justify-content: center;
    }
    .container { width: 100%; max-width: 820px; }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 24px;
      padding-bottom: 16px;
      border-bottom: 1px solid var(--card-border);
    }
    .brand { display: flex; align-items: center; gap: 12px; }
    .brand-icon {
      width: 42px; height: 42px;
      background: linear-gradient(135deg, var(--cyan), var(--blue));
      border-radius: 12px;
      display: flex; align-items: center; justify-content: center;
      font-weight: 700; font-size: 20px; color: #070e17;
      box-shadow: var(--glow);
    }
    .brand-title h1 { font-size: 22px; font-weight: 700; letter-spacing: -0.5px; }
    .brand-title p { font-size: 13px; color: var(--muted); }
    .nav-tabs {
      display: flex;
      gap: 8px;
      background: rgba(10, 20, 30, 0.8);
      padding: 6px;
      border-radius: 14px;
      margin-bottom: 24px;
      border: 1px solid rgba(255,255,255,0.06);
      overflow-x: auto;
    }
    .tab-btn {
      background: transparent;
      border: none;
      color: var(--muted);
      padding: 10px 18px;
      border-radius: 10px;
      cursor: pointer;
      font-family: inherit;
      font-weight: 600;
      font-size: 14px;
      transition: all 0.25s ease;
      white-space: nowrap;
    }
    .tab-btn.active {
      background: rgba(104, 218, 234, 0.15);
      color: var(--cyan);
      box-shadow: 0 2px 10px rgba(104, 218, 234, 0.2);
    }
    .tab-pane { display: none; }
    .tab-pane.active { display: block; animation: fadeIn 0.3s ease; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(6px); } to { opacity: 1; transform: translateY(0); } }

    .grid-2 { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 16px; margin-bottom: 20px; }
    .grid-4 { display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 14px; margin-bottom: 20px; }

    .card {
      background: var(--card);
      border: 1px solid var(--card-border);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border-radius: 18px;
      padding: 20px;
      transition: all 0.25s ease;
    }
    .card:hover { border-color: rgba(104, 218, 234, 0.35); box-shadow: var(--glow); }

    .stat-label { font-size: 12px; color: var(--muted); font-weight: 600; text-transform: uppercase; letter-spacing: 0.8px; }
    .stat-val { font-size: 28px; font-weight: 700; margin: 6px 0; color: var(--text); font-family: 'JetBrains Mono', monospace; }
    .stat-desc { font-size: 13px; color: var(--dim); }

    .form-group { margin-bottom: 18px; }
    label { display: block; font-size: 13px; font-weight: 600; color: var(--muted); margin-bottom: 8px; }
    input[type="text"], input[type="password"], input[type="number"], select {
      width: 100%;
      background: rgba(10, 20, 30, 0.7);
      border: 1px solid rgba(255,255,255,0.12);
      padding: 12px 14px;
      border-radius: 10px;
      color: var(--text);
      font-family: inherit;
      font-size: 14px;
      outline: none;
      transition: border-color 0.2s;
    }
    input:focus, select:focus { border-color: var(--cyan); box-shadow: 0 0 10px rgba(104, 218, 234, 0.2); }
    .row { display: flex; gap: 14px; }
    .col { flex: 1; }

    .btn {
      background: linear-gradient(135deg, var(--cyan), var(--blue));
      color: #070e17;
      border: none;
      padding: 12px 24px;
      border-radius: 12px;
      font-family: inherit;
      font-weight: 700;
      font-size: 14px;
      cursor: pointer;
      display: inline-flex;
      align-items: center;
      gap: 8px;
      box-shadow: var(--glow);
      transition: all 0.2s ease;
    }
    .btn:hover { transform: translateY(-1px); filter: brightness(1.1); }
    .btn-secondary {
      background: rgba(255,255,255,0.08);
      color: var(--text);
      box-shadow: none;
    }
    .btn-secondary:hover { background: rgba(255,255,255,0.14); }

    .badge {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 20px;
      font-size: 11px;
      font-weight: 700;
      text-transform: uppercase;
    }
    .badge-online { background: rgba(117, 219, 160, 0.2); color: var(--green); border: 1px solid rgba(117, 219, 160, 0.3); }
    .badge-offline { background: rgba(255, 127, 131, 0.2); color: var(--red); border: 1px solid rgba(255, 127, 131, 0.3); }

    .switch-wrap { display: flex; align-items: center; justify-content: space-between; padding: 6px 0; }
    .toast {
      position: fixed; bottom: 24px; right: 24px;
      background: var(--card); border: 1px solid var(--cyan);
      color: var(--text); padding: 14px 20px; border-radius: 12px;
      box-shadow: var(--glow); display: none; z-index: 100;
      animation: fadeIn 0.3s ease;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="brand">
        <div class="brand-icon">M</div>
        <div class="brand-title">
          <h1>MeteorS3 Hub</h1>
          <p>Waveshare ESP32-S3-LCD-2 &bull; Firmware v4.0</p>
        </div>
      </div>
      <div>
        <span id="connBadge" class="badge badge-offline">Offline</span>
      </div>
    </header>

    <div class="nav-tabs">
      <button class="tab-btn active" onclick="switchTab('dashboard')">Live Dashboard</button>
      <button class="tab-btn" onclick="switchTab('wifi')">WiFi & Network</button>
      <button class="tab-btn" onclick="switchTab('location')">Location & Time</button>
      <button class="tab-btn" onclick="switchTab('display')">Display & Motion</button>
      <button class="tab-btn" onclick="switchTab('mqtt')">Home Assistant / MQTT</button>
      <button class="tab-btn" onclick="switchTab('system')">OTA Update</button>
    </div>

    <!-- TAB 1: DASHBOARD -->
    <div id="tab-dashboard" class="tab-pane active">
      <div class="grid-4">
        <div class="card">
          <div class="stat-label">Temperature</div>
          <div class="stat-val" id="valTemp">-- &deg;C</div>
          <div class="stat-desc" id="valFeels">Feels --</div>
        </div>
        <div class="card">
          <div class="stat-label">Air Quality</div>
          <div class="stat-val" id="valAqi">--</div>
          <div class="stat-desc" id="valAqiCat">AQI</div>
        </div>
        <div class="card">
          <div class="stat-label">Humidity</div>
          <div class="stat-val" id="valHumidity">--%</div>
          <div class="stat-desc" id="valDew">Dew --</div>
        </div>
        <div class="card">
          <div class="stat-label">Pressure</div>
          <div class="stat-val" id="valPressure">--</div>
          <div class="stat-desc" id="valTrend">hPa</div>
        </div>
      </div>

      <div class="grid-2">
        <div class="card">
          <div class="stat-label">Device Telemetry</div>
          <p style="margin: 12px 0 6px; font-size: 14px;"><strong>Battery:</strong> <span id="valBat">--%</span> (<span id="valBatV">-- V</span>)</p>
          <p style="margin: 6px 0; font-size: 14px;"><strong>WiFi RSSI:</strong> <span id="valRssi">-- dBm</span></p>
          <p style="margin: 6px 0; font-size: 14px;"><strong>Free Heap / PSRAM:</strong> <span id="valHeap">-- KB</span> / <span id="valPsram">-- KB</span></p>
          <p style="margin: 6px 0; font-size: 14px;"><strong>Uptime:</strong> <span id="valUptime">--</span></p>
          <div style="margin-top: 16px; display: flex; gap: 10px;">
            <button class="btn" onclick="triggerRefresh()">Sync Open-Meteo</button>
            <button class="btn btn-secondary" onclick="rebootDevice()">Reboot Device</button>
          </div>
        </div>

        <div class="card">
          <div class="stat-label">Weather Intelligence</div>
          <h3 id="valInsightPrimary" style="margin: 10px 0 6px; font-size: 16px; color: var(--cyan);">Loading intelligence...</h3>
          <p id="valInsightSec" style="font-size: 14px; color: var(--muted); margin-bottom: 12px;">--</p>
          <div style="padding-top: 10px; border-top: 1px solid rgba(255,255,255,0.08);">
            <div class="stat-label">Active Alerts</div>
            <div id="alertsList" style="margin-top: 8px; font-size: 14px; color: var(--yellow);">None</div>
          </div>
        </div>
      </div>
    </div>

    <!-- TAB 2: WIFI -->
    <div id="tab-wifi" class="tab-pane">
      <div class="card">
        <h2 style="margin-bottom: 18px;">WiFi Configuration</h2>
        <form id="wifiForm" onsubmit="saveConfig(event)">
          <div class="form-group">
            <label>WiFi SSID</label>
            <input type="text" id="cfgSsid" name="ssid" required placeholder="Your WiFi network name">
          </div>
          <div class="form-group">
            <label>WiFi Password</label>
            <input type="password" id="cfgPass" name="pass" placeholder="Password">
          </div>
          <button type="submit" class="btn">Save & Connect</button>
        </form>
      </div>
    </div>

    <!-- TAB 3: LOCATION -->
    <div id="tab-location" class="tab-pane">
      <div class="card">
        <h2 style="margin-bottom: 18px;">Location & Coordinates</h2>
        <form id="locForm" onsubmit="saveConfig(event)">
          <div class="form-group">
            <label>Location Name (Header display)</label>
            <input type="text" id="cfgLoc" name="loc" required placeholder="e.g. HOME, TOKYO, NYC">
          </div>
          <div class="row">
            <div class="col form-group">
              <label>Latitude</label>
              <input type="number" step="0.000001" id="cfgLat" name="lat" required>
            </div>
            <div class="col form-group">
              <label>Longitude</label>
              <input type="number" step="0.000001" id="cfgLon" name="lon" required>
            </div>
          </div>
          <div class="form-group">
            <label>Timezone (IANA)</label>
            <input type="text" id="cfgTz" name="tz" required placeholder="e.g. Asia/Kolkata, America/New_York">
          </div>
          <div class="row">
            <div class="col form-group">
              <label>Temperature Unit</label>
              <select id="cfgTu" name="tu">
                <option value="0">Celsius (&deg;C)</option>
                <option value="1">Fahrenheit (&deg;F)</option>
              </select>
            </div>
            <div class="col form-group">
              <label>Wind Speed Unit</label>
              <select id="cfgWu" name="wu">
                <option value="0">km/h</option>
                <option value="1">mph</option>
                <option value="2">m/s</option>
              </select>
            </div>
            <div class="col form-group">
              <label>Pressure Unit</label>
              <select id="cfgPu" name="pu">
                <option value="0">hPa</option>
                <option value="1">inHg</option>
              </select>
            </div>
          </div>
          <button type="submit" class="btn">Update Location & Units</button>
        </form>
      </div>
    </div>

    <!-- TAB 4: DISPLAY & GESTURES -->
    <div id="tab-display" class="tab-pane">
      <div class="card">
        <h2 style="margin-bottom: 18px;">Display & Motion Settings</h2>
        <form id="dispForm" onsubmit="saveConfig(event)">
          <div class="form-group">
            <label>Backlight Brightness (0 - 255): <span id="lblBright">220</span></label>
            <input type="range" min="10" max="255" id="cfgBl" name="bl" style="width:100%;" oninput="document.getElementById('lblBright').innerText = this.value">
          </div>
          <div class="form-group">
            <label>Screen Timeout (seconds, 0 = never sleep)</label>
            <input type="number" min="0" max="3600" id="cfgTout" name="tout">
          </div>
          <div class="switch-wrap form-group">
            <label>Enable IMU Gestures (Tilt left/right, Shake to Home)</label>
            <input type="checkbox" id="cfgGest" name="gest">
          </div>
          <div class="form-group">
            <label>Gesture Sensitivity (1 - 10): <span id="lblSens">5</span></label>
            <input type="range" min="1" max="10" id="cfgGsens" name="gsens" style="width:100%;" oninput="document.getElementById('lblSens').innerText = this.value">
          </div>
          <div class="switch-wrap form-group">
            <label>Auto-rotate Pages</label>
            <input type="checkbox" id="cfgApg" name="apg">
          </div>
          <div class="form-group">
            <label>Auto-rotate Interval (seconds)</label>
            <input type="number" min="5" max="300" id="cfgApgs" name="apgs">
          </div>
          <button type="submit" class="btn">Save Display Settings</button>
        </form>
      </div>
    </div>

    <!-- TAB 5: MQTT -->
    <div id="tab-mqtt" class="tab-pane">
      <div class="card">
        <h2 style="margin-bottom: 18px;">Home Assistant / MQTT Integration</h2>
        <form id="mqttForm" onsubmit="saveConfig(event)">
          <div class="switch-wrap form-group">
            <label>Enable MQTT Telemetry & Discovery</label>
            <input type="checkbox" id="cfgMqEn" name="mq_en">
          </div>
          <div class="row">
            <div class="col form-group" style="flex:2;">
              <label>Broker Host / IP</label>
              <input type="text" id="cfgMqSrv" name="mq_srv" placeholder="192.168.1.100 or mqtt.local">
            </div>
            <div class="col form-group" style="flex:1;">
              <label>Port</label>
              <input type="number" id="cfgMqPort" name="mq_port" value="1883">
            </div>
          </div>
          <div class="row">
            <div class="col form-group">
              <label>Username (Optional)</label>
              <input type="text" id="cfgMqUsr" name="mq_usr">
            </div>
            <div class="col form-group">
              <label>Password (Optional)</label>
              <input type="password" id="cfgMqPwd" name="mq_pwd">
            </div>
          </div>
          <div class="form-group">
            <label>Topic Prefix</label>
            <input type="text" id="cfgMqPfx" name="mq_pfx" value="weatherstation">
          </div>
          <button type="submit" class="btn">Save MQTT Settings</button>
        </form>
      </div>
    </div>

    <!-- TAB 6: SYSTEM & OTA -->
    <div id="tab-system" class="tab-pane">
      <div class="card">
        <h2 style="margin-bottom: 18px;">Over-The-Air Firmware Update</h2>
        <form method="POST" action="/update" enctype="multipart/form-data">
          <div class="form-group">
            <label>Select compiled .bin firmware binary</label>
            <input type="file" name="update" accept=".bin" style="color:var(--muted); padding:10px; background:rgba(0,0,0,0.3); border-radius:8px; width:100%;">
          </div>
          <button type="submit" class="btn">Upload & Flash Firmware</button>
        </form>
      </div>
    </div>
  </div>

  <div id="toast" class="toast">Settings saved successfully!</div>

  <script>
    function switchTab(id) {
      document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
      document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
      event.target.classList.add('active');
      document.getElementById('tab-' + id).classList.add('active');
    }

    function showToast(msg) {
      const t = document.getElementById('toast');
      t.innerText = msg;
      t.style.display = 'block';
      setTimeout(() => { t.style.display = 'none'; }, 3000);
    }

    async function loadStatus() {
      try {
        const res = await fetch('/api/status');
        const d = await res.json();

        document.getElementById('connBadge').className = 'badge ' + (d.online ? 'badge-online' : 'badge-offline');
        document.getElementById('connBadge').innerText = d.online ? 'Online (' + d.ip + ')' : (d.apMode ? 'AP Setup Mode' : 'Offline');

        document.getElementById('valTemp').innerHTML = (d.temp !== null ? d.temp.toFixed(1) : '--') + ' &deg;C';
        document.getElementById('valFeels').innerText = 'Feels ' + (d.apparent !== null ? d.apparent.toFixed(1) : '--') + ' C';
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

      // Handle checkboxes
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
          showToast('Settings saved & applied!');
          loadConfig();
        }
      } catch(err) { alert('Failed to save settings: ' + err); }
    }

    async function triggerRefresh() {
      await fetch('/api/refresh', { method: 'POST' });
      showToast('Refresh triggered!');
      setTimeout(loadStatus, 1500);
    }

    async function rebootDevice() {
      if (confirm('Reboot Weather Station?')) {
        await fetch('/api/reboot', { method: 'POST' });
        showToast('Rebooting...');
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
