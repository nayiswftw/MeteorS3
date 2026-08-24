#pragma once

#include <Arduino.h>

namespace svc {

void mqttInit();
void mqttService();
bool isMqttConnected();

}  // namespace svc
