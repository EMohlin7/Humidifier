#pragma once
#include <Arduino.h>
#include <MQTT.h>


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






