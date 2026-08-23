#include "hal/Imu.h"
#include "Config.h"
#include <Arduino.h>
#include <Wire.h>

namespace hal {

static bool s_imuFound = false;

// QMI8658 Register Map
constexpr uint8_t QMI8658_REG_WHO_AM_I = 0x00;
constexpr uint8_t QMI8658_REG_CTRL1    = 0x02;
constexpr uint8_t QMI8658_REG_CTRL2    = 0x03;
constexpr uint8_t QMI8658_REG_CTRL7    = 0x08;
constexpr uint8_t QMI8658_REG_AX_L     = 0x35;

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
    Serial.println("[imu] QMI8658 ready");
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

    // Default sensitivity: 8g (4096 LSB/g), 512 dps (64 LSB/dps)
    out.accelX = (float)ax / 4096.0f;
    out.accelY = (float)ay / 4096.0f;
    out.accelZ = (float)az / 4096.0f;
    out.gyroX  = (float)gx / 64.0f;
    out.gyroY  = (float)gy / 64.0f;
    out.gyroZ  = (float)gz / 64.0f;

    return true;
}

bool isImuAvailable() {
    return s_imuFound;
}

}  // namespace hal
