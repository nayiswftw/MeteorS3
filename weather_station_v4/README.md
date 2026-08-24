# WEATHER STATION V4 (Arduino IDE Edition)

**Hardware:** Waveshare ESP32-S3-LCD-2 (ESP32-S3R8, 16MB Flash, 8MB PSRAM, ST7789T3 240x320 IPS)

---

## 🛠️ Arduino IDE Setup

### 1. Required Board Package
- Open **Arduino IDE** -> **Boards Manager** (left sidebar).
- Install **esp32 by Espressif Systems** (v3.x or latest).

### 2. Required Libraries (Arduino Library Manager)
Install each of these via **Library Manager** (`Ctrl+Shift+I` / `Cmd+Shift+I`):
- **ArduinoJson** (by Benoit Blanchon, v7.x)
- **GFX Library for Arduino** (by Moon On Our Nation, v1.6.x+)
- **lvgl** (by LVGL / kisvegabor, v9.5.x+)

> **Note on LVGL Configuration:**  
> Ensure your `lv_conf.h` is placed in `Documents/Arduino/libraries/lv_conf.h` (or inside the project `src/lv_conf.h`). A pre-configured `lv_conf.h` is provided in `src/lv_conf.h`.

---

## ⚙️ Board Settings in Arduino IDE (Tools Menu)

When compiling or uploading, set the following under **Tools**:

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
| **Core Debug Level** | `None` (or `Info`) |

---

## 🚀 How to Run

1. Open `weather_station_v4.ino` in Arduino IDE.
2. Edit `src/Config.h` to set your WiFi SSID, password, coordinates, and units:
   ```cpp
   constexpr char WIFI_SSID[]     = "Your_WiFi_SSID";
   constexpr char WIFI_PASSWORD[] = "Your_WiFi_Password";
   constexpr double LATITUDE      = 30.000000;
   constexpr double LONGITUDE     = 75.000000;
   ```
3. Connect your ESP32-S3 device via USB-C.
4. Select the COM port under **Tools -> Port**.
5. Click **Upload** (or `Ctrl+U`).
6. Open **Serial Monitor** at **115200 baud**.

---

## 📁 Project Structure

```text
weather_station_v4/
├── weather_station_v4.ino     # Main Arduino sketch entry point (setup & loop)
├── README.md                  # Arduino IDE guide & documentation
└── src/
    ├── Config.h               # User configuration (WiFi, GPS, units, pins)
    ├── lv_conf.h              # LVGL 9.x configuration
    ├── core/                  # Data structures, formatting, weather engine, state
    │   ├── Format.h / .cpp
    │   ├── State.h / .cpp
    │   ├── Types.h
    │   └── Weather.h / .cpp
    ├── hal/                   # Hardware abstraction (Display, Input, Battery, IMU)
    │   ├── Battery.h / .cpp
    │   ├── Display.h / .cpp
    │   ├── Imu.h / .cpp
    │   └── Input.h / .cpp
    ├── services/              # Networking, Time, API, Storage/SD
    │   ├── Api.h / .cpp
    │   ├── Network.h / .cpp
    │   ├── Storage.h / .cpp
    │   └── TimeSync.h / .cpp
    └── ui/                    # LVGL screen management, themes, widgets & screens
        ├── Icons.h / .cpp
        ├── Screen.h / .cpp
        ├── Theme.h
        ├── Widgets.h / .cpp
        └── screens/           # Modular screen implementations (Home, Hourly, Week, etc.)
```
