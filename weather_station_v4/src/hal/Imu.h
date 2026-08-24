#pragma once

/*
 * IMU HAL — QMI8658 6-axis accelerometer & gyroscope.
 */

namespace hal {

struct ImuData {
    float accelX = 0.0f; // in g
    float accelY = 0.0f;
    float accelZ = 0.0f;
    float gyroX  = 0.0f; // in dps
    float gyroY  = 0.0f;
    float gyroZ  = 0.0f;
};

/// Initialize I2C and detect QMI8658 IMU sensor.
bool imuInit();

/// Read latest acceleration and angular velocity data.
bool imuRead(ImuData& out);

/// Returns true if IMU hardware was detected on boot.
bool isImuAvailable();

}  // namespace hal
