#include "src/hal/Imu.h"
#include "src/hal/Backlight.h"
#include "src/Config.h"
#include "src/core/State.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

namespace hal {

static bool    s_imuFound = false;
static uint8_t s_imuAddr  = 0x6B;

// QMI8658 Register Map
constexpr uint8_t QMI8658_REG_WHO_AM_I = 0x00;
constexpr uint8_t QMI8658_REG_CTRL1    = 0x02;
constexpr uint8_t QMI8658_REG_CTRL2    = 0x03;
constexpr uint8_t QMI8658_REG_CTRL3    = 0x04;
constexpr uint8_t QMI8658_REG_CTRL7    = 0x08;
constexpr uint8_t QMI8658_REG_AX_L     = 0x35;

// Gesture state variables
enum class TiltState { NEUTRAL, TILTED_RIGHT, TILTED_LEFT };
static TiltState   s_tiltState         = TiltState::NEUTRAL;
static GestureType s_lastGesture       = GestureType::NONE;
static uint32_t    s_lastGestureTimeMs = 0;
static uint32_t    s_lastSampleMs      = 0;
static bool        s_motionDetected    = false;

static float s_filtAx = 0.0f;
static float s_filtAy = 0.0f;
static float s_filtAz = 1.0f;
static float s_prevAx = 0.0f;
static float s_prevAy = 0.0f;
static float s_prevAz = 1.0f;

static bool writeRegister(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(s_imuAddr);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

static bool checkChip(uint8_t addr) {
    Wire.beginTransmission(addr);
    Wire.write(QMI8658_REG_WHO_AM_I);
    if (Wire.endTransmission() != 0) return false;

    Wire.requestFrom(addr, (uint8_t)1);
    if (!Wire.available()) return false;

    uint8_t chipId = Wire.read();
    return (chipId == 0x05);
}

bool imuInit() {
    struct PinPair { int sda; int scl; };
    const PinPair candidatePins[] = {
        { config::IMU_SDA, config::IMU_SCL }, // 15, 7 (Waveshare ESP32-S3-LCD-2 / Touch-LCD-2)
        { 15, 7 },
        { 6, 7 },
        { 11, 12 },
        { 4, 5 },
        { 1, 3 }
    };

    s_imuFound = false;

    for (const auto& pins : candidatePins) {
        Wire.end();
        Wire.begin(pins.sda, pins.scl, 400000);

        if (checkChip(config::IMU_ADDR)) {
            s_imuAddr = config::IMU_ADDR;
            s_imuFound = true;
            Serial.printf("[imu] QMI8658 found on SDA=%d, SCL=%d (Addr=0x%02X)\n", pins.sda, pins.scl, s_imuAddr);
            break;
        } else if (checkChip(0x6A)) {
            s_imuAddr = 0x6A;
            s_imuFound = true;
            Serial.printf("[imu] QMI8658 found on SDA=%d, SCL=%d (Addr=0x6A)\n", pins.sda, pins.scl);
            break;
        } else if (checkChip(0x6B)) {
            s_imuAddr = 0x6B;
            s_imuFound = true;
            Serial.printf("[imu] QMI8658 found on SDA=%d, SCL=%d (Addr=0x6B)\n", pins.sda, pins.scl);
            break;
        }
    }

    if (!s_imuFound) {
        Serial.println("[imu] QMI8658 not detected on I2C (probed candidate pins)");
        return false;
    }

    // 1. CTRL1: 0x60 -> Bit 6 = 1 (Address Auto-Increment enable for multi-byte read), Bit 5 = 1 (400kHz)
    writeRegister(QMI8658_REG_CTRL1, 0x60);
    delay(10);

    // 2. CTRL2: 0x23 -> Accel +/-8g full scale, 125 Hz Output Data Rate (ODR)
    writeRegister(QMI8658_REG_CTRL2, 0x23);

    // 3. CTRL3: 0x53 -> Gyro +/-512 dps full scale, 125 Hz Output Data Rate (ODR)
    writeRegister(QMI8658_REG_CTRL3, 0x53);

    // 4. CTRL7: 0x03 -> Enable both Accelerometer and Gyroscope
    writeRegister(QMI8658_REG_CTRL7, 0x03);
    delay(20);

    s_tiltState = TiltState::NEUTRAL;
    Serial.printf("[imu] QMI8658 ready on 0x%02X (Auto-Inc Enabled, 125Hz 6-Axis)\n", s_imuAddr);
    return true;
}

bool imuRead(ImuData& out) {
    if (!s_imuFound) return false;

    Wire.beginTransmission(s_imuAddr);
    Wire.write(QMI8658_REG_AX_L);
    if (Wire.endTransmission() != 0) return false;

    Wire.requestFrom(s_imuAddr, (uint8_t)12);
    if (Wire.available() < 12) return false;

    int16_t ax = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t ay = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t az = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t gx = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t gy = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t gz = (int16_t)(Wire.read() | (Wire.read() << 8));

    // Sensitivity: +/-8g (4096 LSB/g), +/-512 dps (64 LSB/dps)
    out.accelX = (float)ax / 4096.0f;
    out.accelY = (float)ay / 4096.0f;
    out.accelZ = (float)az / 4096.0f;
    out.gyroX  = (float)gx / 64.0f;
    out.gyroY  = (float)gy / 64.0f;
    out.gyroZ  = (float)gz / 64.0f;

    return true;
}

void imuService() {
    if (!s_imuFound) return;

    uint32_t now = millis();
    if (now - s_lastSampleMs < 20) return; // 50 Hz sampling rate
    s_lastSampleMs = now;

    ImuData d;
    if (!imuRead(d)) return;

    // Low-pass filter for smooth gravity vector tracking (alpha = 0.25)
    constexpr float alpha = 0.25f;
    s_filtAx = alpha * d.accelX + (1.0f - alpha) * s_filtAx;
    s_filtAy = alpha * d.accelY + (1.0f - alpha) * s_filtAy;
    s_filtAz = alpha * d.accelZ + (1.0f - alpha) * s_filtAz;

    // Delta acceleration for physical motion wake-up
    float deltaMotion = fabsf(d.accelX - s_prevAx) + fabsf(d.accelY - s_prevAy) + fabsf(d.accelZ - s_prevAz);
    s_prevAx = d.accelX;
    s_prevAy = d.accelY;
    s_prevAz = d.accelZ;

    if (deltaMotion > 0.22f || fabsf(d.gyroX) > 35.0f || fabsf(d.gyroY) > 35.0f) {
        s_motionDetected = true;
        // Subtle motion wakes up the display smoothly
        backlightResetInactivity();
    } else {
        s_motionDetected = false;
    }

    if (!state::config().enableGestures) return;

    // Dynamic thresholds based on sensitivity setting (1 - 10)
    uint8_t sens = constrain(state::config().gestureSensitivity, 1, 10);
    float tiltThreshold   = 0.52f - (sens * 0.028f); // 0.24g to 0.49g
    float returnThreshold = 0.16f;                    // Return-to-center threshold
    float shakeGyroThresh = 280.0f - (sens * 14.0f);  // 140 to 266 dps

    // 1. Shake Detection (high angular velocity in any axis)
    float gyroMag = sqrtf(d.gyroX * d.gyroX + d.gyroY * d.gyroY + d.gyroZ * d.gyroZ);
    if (gyroMag > shakeGyroThresh && (now - s_lastGestureTimeMs > 600)) {
        s_lastGesture = GestureType::SHAKE;
        s_lastGestureTimeMs = now;
        s_tiltState = TiltState::NEUTRAL;
        backlightResetInactivity();
        return;
    }

    // 2. Hysteresis Tilt State Machine (requires returning towards center before next tilt)
    switch (s_tiltState) {
        case TiltState::NEUTRAL:
            if (s_filtAx > tiltThreshold) {
                s_tiltState = TiltState::TILTED_RIGHT;
                s_lastGesture = GestureType::TILT_RIGHT;
                s_lastGestureTimeMs = now;
                backlightResetInactivity();
            } else if (s_filtAx < -tiltThreshold) {
                s_tiltState = TiltState::TILTED_LEFT;
                s_lastGesture = GestureType::TILT_LEFT;
                s_lastGestureTimeMs = now;
                backlightResetInactivity();
            }
            break;

        case TiltState::TILTED_RIGHT:
            if (s_filtAx < returnThreshold) {
                s_tiltState = TiltState::NEUTRAL;
            }
            break;

        case TiltState::TILTED_LEFT:
            if (s_filtAx > -returnThreshold) {
                s_tiltState = TiltState::NEUTRAL;
            }
            break;
    }
}

GestureType imuReadGesture() {
    GestureType g = s_lastGesture;
    s_lastGesture = GestureType::NONE;
    return g;
}

bool isImuAvailable() {
    return s_imuFound;
}

bool imuMotionDetected() {
    return s_motionDetected;
}

}  // namespace hal

