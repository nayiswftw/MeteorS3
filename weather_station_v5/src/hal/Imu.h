#pragma once

/*
 * IMU HAL — QMI8658 6-axis accelerometer & gyroscope.
 */

#include "src/core/Types.h"

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

/// Update IMU sampling and gesture state machine (call in loop).
void imuService();

/// Read latest acceleration and angular velocity data.
bool imuRead(ImuData& out);

/// Consume any detected gesture (returns NONE if no new gesture).
GestureType imuReadGesture();

/// Returns true if IMU hardware was detected on boot.
bool isImuAvailable();

/// Returns true if significant motion was detected in the last sample.
bool imuMotionDetected();

}  // namespace hal

