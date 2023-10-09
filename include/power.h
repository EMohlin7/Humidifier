#pragma once
#include <Arduino.h>

struct onMonitor{
    bool on;
    SemaphoreHandle_t mutex;
};

struct onMonitor* createOnMonitor(bool defaultVal);

bool _isOn(struct onMonitor* monitor);
void _setOn(struct onMonitor* monitor, bool on);