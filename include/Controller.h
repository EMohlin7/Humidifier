#pragma once
#include <Arduino.h>
#include "Tasks.h"
#include <MQTT.h>


class Controller{
    public:
        Controller(const struct controllerPins* pins, MQTTClient* client);
        ~Controller();

        void turnOn(bool on);

        /*Sets the mode and returns the corresponding pwm value*/
        uint8_t setMode(uint8_t mode);

        /*Toggles the mode and returns the corresponding pwm value*/
        uint8_t toggleMode();
        void setPwm(uint8_t val);
        uint8_t getPwm(uint32_t maxWaitMS);

        bool isOn();
        bool waterTooLow();

        void onWaterChanged();

        void onMqttMsg(const char* topic, const char* payload);

    private:

        void startLowWaterBlink();

        MQTTClient* mqttClient;

        QueueHandle_t pwmQ;
        QueueHandle_t fanQ;
        QueueHandle_t mqttSendQ;
        uint8_t mode;
        struct controllerPins pins;
        struct onMonitor* onMonitor;
        struct speedMonitor* pwmMonitor;

        
};





//BaseType_t initController(struct controllerPins* pins);





