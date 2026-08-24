#include "src/hal/Imu.h"
#include "src/hal/Backlight.h"
#include "src/Config.h"
#include "src/core/State.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

namespace hal {

static bool s_imuFound = false;

// QMI8658 Register Map
constexpr uint8_t QMI8658_REG_WHO_AM_I = 0x00;
constexpr uint8_t QMI8658_REG_CTRL1    = 0x02;
constexpr uint8_t QMI8658_REG_CTRL2    = 0x03;
constexpr uint8_t QMI8658_REG_CTRL7    = 0x08;
constexpr uint8_t QMI8658_REG_AX_L     = 0x35;

// Gesture state variables
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

bool imuInit() {
    Wire.begin(config::IMU_SDA, config::IMU_SCL, 400000);

    Wire.beginTransmission(config::IMU_ADDR);
    Wire.write(QMI8658_REG_WHO_AM_I);
    if (Wire.endTransmission() != 0) {
        Serial.println("[imu] QMI8658 not detected on I2C");
        s_imuFound = false;
        return false;
    }

    Wire.requestFrom((uint8_t)config::IMU_ADDR, (uint8_t)1);
    if (!Wire.available()) {
        s_imuFound = false;
        return false;
    }

    uint8_t chipId = Wire.read();
    if (chipId != 0x05) {
        Serial.printf("[imu] unexpected WHO_AM_I: 0x%02X\n", chipId);
        s_imuFound = false;
        return false;
    }

    // Enable Accelerometer and Gyroscope in CTRL7 (0x03 = both enabled)
    Wire.beginTransmission(config::IMU_ADDR);
    Wire.write(QMI8658_REG_CTRL7);
    Wire.write(0x03);
    Wire.endTransmission();

    s_imuFound = true;
    Serial.println("[imu] QMI8658 ready with gesture recognition engine");
    return true;
}

bool imuRead(ImuData& out) {
    if (!s_imuFound) return false;

    Wire.beginTransmission(config::IMU_ADDR);
    Wire.write(QMI8658_REG_AX_L);
    if (Wire.endTransmission() != 0) return false;

    Wire.requestFrom((uint8_t)config::IMU_ADDR, (uint8_t)12);
    if (Wire.available() < 12) return false;

    int16_t ax = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t ay = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t az = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t gx = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t gy = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t gz = (int16_t)(Wire.read() | (Wire.read() << 8));

    // Sensitivity: 8g (4096 LSB/g), 512 dps (64 LSB/dps)
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
    if (now - s_lastSampleMs < 20) return; // 50 Hz sampling
    s_lastSampleMs = now;

    ImuData d;
    if (!imuRead(d)) return;

    // Low-pass filter (alpha = 0.25)
    constexpr float alpha = 0.25f;
    s_filtAx = alpha * d.accelX + (1.0f - alpha) * s_filtAx;
    s_filtAy = alpha * d.accelY + (1.0f - alpha) * s_filtAy;
    s_filtAz = alpha * d.accelZ + (1.0f - alpha) * s_filtAz;

    // Delta acceleration for motion wake-up
    float deltaMotion = fabsf(d.accelX - s_prevAx) + fabsf(d.accelY - s_prevAy) + fabsf(d.accelZ - s_prevAz);
    s_prevAx = d.accelX;
    s_prevAy = d.accelY;
    s_prevAz = d.accelZ;

    if (deltaMotion > 0.25f || fabsf(d.gyroX) > 40.0f || fabsf(d.gyroY) > 40.0f) {
        s_motionDetected = true;
        // Motion wakes up display
        backlightResetInactivity();
    } else {
        s_motionDetected = false;
    }

    if (!state::config().enableGestures) return;

    // Cooldown check (500ms debounce between gestures)
    if (now - s_lastGestureTimeMs < 500) return;

    // Calculate dynamic thresholds based on sensitivity (1 - 10)
    uint8_t sens = constrain(state::config().gestureSensitivity, 1, 10);
    float tiltThreshold = 0.55f - (sens * 0.035f); // 0.20g to 0.51g
    float shakeGyroThresh = 280.0f - (sens * 15.0f); // 130 to 265 dps

    // 1. Shake Detection (high angular velocity)
    float gyroMag = sqrtf(d.gyroX * d.gyroX + d.gyroY * d.gyroY + d.gyroZ * d.gyroZ);
    if (gyroMag > shakeGyroThresh) {
        s_lastGesture = GestureType::SHAKE;
        s_lastGestureTimeMs = now;
        return;
    }

    // 2. Tilt Navigation (accelX tilt left/right)
    if (s_filtAx > tiltThreshold) {
        s_lastGesture = GestureType::TILT_RIGHT;
        s_lastGestureTimeMs = now;
    } else if (s_filtAx < -tiltThreshold) {
        s_lastGesture = GestureType::TILT_LEFT;
        s_lastGestureTimeMs = now;
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

