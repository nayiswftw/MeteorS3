#pragma once

/*
 * Battery HAL — ADC voltage measurement and state-of-charge calculation.
 */

namespace hal {

void batteryInit();

/// Measured battery voltage in volts (e.g. 3.70 - 4.20V).
float readBatteryVoltage();

/// Estimated battery percentage (0 - 100%).
int readBatteryPercent();

}  // namespace hal
