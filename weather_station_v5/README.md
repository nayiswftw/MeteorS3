# 🌦️ METEORS3 — WEATHER STATION V4 (Waveshare Edition)

**Hardware:** Waveshare ESP32-S3-LCD-2 (ESP32-S3R8, 16MB Flash, 8MB PSRAM, ST7789T3 240x320 IPS, QMI8658 6-Axis IMU)

An elevated, buttery-smooth IoT weather station with **dynamic particle weather effects, 6-axis IMU gesture navigation, FreeRTOS async fetching, responsive glassmorphic Web Portal, and Home Assistant MQTT integration**.

---

## ✨ Feature Highlights

- **Dynamic Weather Motion & Particle Engine**: Procedural falling rain streaks with splashes, drifting snow particles, pulsing glowing sun rays, cloud drift, and thunderstorm lightning flashes.
- **6-Axis IMU Gesture Navigation (QMI8658)**:
  - **Tilt Right**: Next screen.
  - **Tilt Left**: Previous screen.
  - **Shake**: Return directly to the Home screen.
  - **Wake on Motion**: Screen wakes up automatically when picked up or moved.
- **Multi-Core FreeRTOS Engine**: HTTP network operations and Open-Meteo JSON parsing run in a dedicated background task pinned to **Core 0**, while the UI loop runs at **60 FPS on Core 1** with zero frame drops or freezing.
- **Smart Backlight & Power Management**: Hardware LEDC PWM brightness control with smooth fading, customizable sleep timeout, and auto-dimming.
- **Embedded Glassmorphism Web Portal & Captive Portal**:
  - Automatically launches AP `WeatherStation-Setup` if WiFi fails or BOOT button is held on startup.
  - Live configuration: change WiFi, GPS coordinates, location name, units, backlight brightness, timeout, and tilt sensitivity on the fly without recompiling!
  - Real-time device telemetry dashboard & Over-The-Air (Web OTA) firmware flashing.
- **Smart Home / Home Assistant Integration**: Native MQTT client with Home Assistant Auto-Discovery and state telemetry publishing.
- **Celestial & Lunar Phase Renderer**: High-accuracy procedural moon illumination rendering and solar tracking.

---

## 🎮 Physical & Gesture Controls

| Action | Control | Result |
|---|---|---|
| **Tilt Right** | IMU Gesture | Slide to next screen |
| **Tilt Left** | IMU Gesture | Slide to previous screen |
| **Shake** | IMU Gesture | Jump to Home dashboard |
| **Single Click** | BOOT Button (GPIO 0) | Next screen / Wake display |
| **Double Click** | BOOT Button (GPIO 0) | Toggle backlight sleep / wake |
| **Long Press** | BOOT Button (GPIO 0) | Jump to Home dashboard |
| **Hold on Boot** | BOOT Button (GPIO 0) | Force AP Setup Mode (`WeatherStation-Setup`) |

---

## 🛠️ Arduino IDE Setup

### 1. Required Board Package
- Open **Arduino IDE** -> **Boards Manager** (left sidebar).
- Install **esp32 by Espressif Systems** (v3.x or latest).

### 2. Required Libraries (Arduino Library Manager)
Install each of these via **Library Manager** (`Ctrl+Shift+I`):
- **ArduinoJson** (by Benoit Blanchon, v7.x)
- **GFX Library for Arduino** (by Moon On Our Nation, v1.6.x+)
- **lvgl** (by LVGL / kisvegabor, v9.5.x+)

---

## ⚙️ Board Settings in Arduino IDE (Tools Menu)

| Option | Setting |
|---|---|
| **Board** | `ESP32S3 Dev Module` |
| **Flash Size** | `16MB (128Mb)` |
| **Partition Scheme** | `16M Flash (3MB APP/9.9MB FATFS)` (or `Default 16MB with spiffs`) |
| **PSRAM** | `OPI PSRAM` |
| **USB CDC On Boot** | `Enabled` |
| **Upload Mode** | `UART0 / Hardware CDC` |
| **Upload Speed** | `921600` |
| **CPU Frequency** | `240MHz (WiFi)` |

---

## 🌐 Web Portal & Initial Setup

1. On first boot (or if unconfigured), connect your phone or laptop to the WiFi AP:
   - **SSID:** `WeatherStation-Setup`
   - **Password:** `12345678`
2. Open your browser to `http://192.168.4.1` (or follow the Captive Portal prompt).
3. Set your home WiFi, coordinates, and preferences. Click **Save & Connect**.
4. Once connected to your home WiFi, open `http://<device_ip>` to access the live glassmorphic telemetry dashboard anytime!

---

## 📁 Project Structure

```text
weather_station_v4/
├── weather_station_v4.ino     # Main Arduino sketch entry point (multi-core setup & loop)
├── README.md                  # Comprehensive documentation
└── src/
    ├── Config.h               # Hardware pin definitions & compile-time defaults
    ├── lv_conf.h              # LVGL 9.x configuration
    ├── core/                  # Data structures, state machine, formatters
    │   ├── Format.h / .cpp    # Metric/Imperial value formatters
    │   ├── State.h / .cpp     # Thread-safe FreeRTOS mutex state engine
    │   ├── Types.h            # Core data types, gestures, telemetry structs
    │   └── Weather.h / .cpp   # Weather intelligence & alert synthesis
    ├── hal/                   # Hardware Abstraction Layer
    │   ├── Backlight.h / .cpp # LEDC PWM smooth brightness & sleep timer
    │   ├── Battery.h / .cpp   # ADC voltage monitoring
    │   ├── Display.h / .cpp   # ST7789 IPS & LVGL 9 display driver
    │   ├── Imu.h / .cpp       # QMI8658 6-axis gesture & motion engine
    │   └── Input.h / .cpp     # Multi-click button state machine
    ├── services/              # Networking, Background Workers & Storage
    │   ├── Api.h / .cpp       # Core 0 FreeRTOS Open-Meteo async fetcher
    │   ├── Mqtt.h / .cpp      # Home Assistant MQTT Auto-Discovery bridge
    │   ├── Network.h / .cpp   # WiFi state machine & Captive Portal DNS
    │   ├── Storage.h / .cpp   # NVS Preferences & SD card logging
    │   ├── TimeSync.h / .cpp  # SNTP network time synchronization
    │   └── WebServer.h / .cpp # Responsive Glassmorphic Web UI & OTA
    └── ui/                    # Visual Presentation Layer
        ├── Animations.h /.cpp # Dynamic weather particle engine & transitions
        ├── Icons.h / .cpp     # Procedural weather art & lunar phase renderer
        ├── Screen.h / .cpp    # Screen registry & transition manager
        ├── Theme.h            # Glassmorphic color palette & layout tokens
        ├── Widgets.h / .cpp   # Frosted glass cards, gauges, charts, banners
        └── screens/           # 12 Modular animated screens
```

