#pragma once
#include <Arduino.h>



//const QueueHandle_t pwmQ = xQueueCreate(5, sizeof(uint8_t));
//const QueueHandle_t powerQ = xQueueCreate(5, sizeof(bool));

struct taskArgs{
    uint8_t pin;
    QueueHandle_t q;
};

struct controllerPins{
    uint8_t pwmPin; // Pin to controll ammount of moisture
    uint8_t fanPin; // Pin for fan, on or off
    uint8_t pTouchPin; // Pin used to read touch input from power button
    uint8_t mTouchPin; // Pin used to read touch input from mode button
    uint8_t waterPin; // Pin used for interrupt when water level gets too low
    uint8_t rLPin; // Red pin for status led
    uint8_t gLPin; // Green pin for status led
    uint8_t bLPin; // Blue pin for status led
};

BaseType_t initController(struct controllerPins* pins, QueueHandle_t pwmQ, QueueHandle_t fanQ);



void pwmTask(void* pwmParams);
void fanTask(void* fanPin);

