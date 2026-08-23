#pragma once
#include <Arduino.h>
#include <lvgl.h>

void hardwareBegin();
void displayBegin();
void displayService();

float readBatteryVoltage();
int readBatteryPercent();

bool buttonShortPressed();
bool buttonLongPressed();

lv_obj_t *displayRoot();
