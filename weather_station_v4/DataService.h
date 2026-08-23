#pragma once

#include <Arduino.h>

void dataBegin();
void dataService();

bool refreshWeather();
bool refreshAir();

String localClockText();
String localDateText();
