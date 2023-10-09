#pragma once
#include <Arduino.h>
struct speedMonitor{
    SemaphoreHandle_t mutex;
    uint8_t speed;
};

struct speedMonitor* createSpeedMonitor(uint8_t startVal);
uint8_t getSpeed(struct speedMonitor* monitor, uint32_t maxWaitMS);

// returns true if successfull otherwise false
bool setSpeed(struct speedMonitor* monitor, uint8_t speed, uint32_t  maxWaitMS);