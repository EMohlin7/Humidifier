#include "Controller.h"
#include "power.h"
#include "Modes.h"
#include "MqttTopics.h"
#include "MqttPayloads.h"
#include "SpeedMonitor.h"

#define BLINK_TIME_MS 600
#define BLINK_CYCLES 4
#define FAN_EXTRA_ON_TIME_MS 6000


Controller::Controller(const struct controllerPins* pins, MQTTClient* client) : mqttClient(client)
{
    this->pins = *pins;
    
    onMonitor = createOnMonitor(false);
    pwmMonitor = createSpeedMonitor(0);
    pwmQ = xQueueCreate(5, sizeof(uint8_t));
    mqttSendQ = xQueueCreate(5, sizeof(uint8_t));
    fanQ = xQueueCreate(5, sizeof(uint8_t));
    if(onMonitor == NULL || pwmMonitor == NULL || pwmQ == NULL || fanQ == NULL || mqttSendQ == NULL )//|| initController(pins, pwmQ, fanQ) != pdPASS)
        throw std::exception();
    mqttClient->ref = this;
}

Controller::~Controller()
{
    xSemaphoreTake(onMonitor->mutex, portMAX_DELAY);
    vSemaphoreDelete(onMonitor->mutex);
    free(onMonitor);

    xSemaphoreTake(pwmMonitor->mutex, portMAX_DELAY);
    vSemaphoreDelete(pwmMonitor->mutex);
    free(pwmMonitor);
}

void Controller::onMqttMsg(const char* topic, const char* payload)
{
    if(strncmp(topic, POWER_SET_TOPIC, strlen(POWER_SET_TOPIC)) == 0)
    {
        turnOn(strncmp(payload, POWER_ON_PAYLOAD, strlen(POWER_ON_PAYLOAD)) == 0);
    }
    else if(strncmp(topic, SPEED_SET_TOPIC, strlen(SPEED_SET_TOPIC)) == 0)
    {
        // Speed is zero if no conversion could be made
        uint8_t speed = atoi(payload);
        
        if(speed < PWM_LOW)
            speed = PWM_LOW;
        else if(speed > PWM_MAX)
            speed = PWM_MAX;

        setPwm(speed);
    }
}

void stopFan(TimerHandle_t timer)
{
    //The fan pin is stored as the timer id
    uint32_t pin = (uint32_t)pvTimerGetTimerID(timer);

    digitalWrite(pin, LOW);
}

void Controller::turnOn(bool on) 
{
    static TimerHandle_t fanTimer = xTimerCreate("timer", pdMS_TO_TICKS(FAN_EXTRA_ON_TIME_MS), pdFALSE, (void*)pins.fanPin, stopFan);
    on = on*!waterTooLow();
    
    _setOn(onMonitor, on);
    Serial.println("Turn on");
    if(!on)
        xTimerStart(fanTimer, 0);
    else
    {
        xTimerStop(fanTimer, 0);
        digitalWrite(pins.fanPin, HIGH);
    }
    mqttClient->publish(POWER_STATE_TOPIC, on ? POWER_ON_PAYLOAD : POWER_OFF_PAYLOAD, true, 0);
    
    //Update the output
    setPwm(setMode(0)*on);

    if(waterTooLow())
        startLowWaterBlink();
}


void Controller::setPwm(uint8_t pwm)
{
    if(!isOn() || waterTooLow())
        pwm = 0;
    else if(pwm < PWM_LOW)
        pwm = PWM_LOW;
    else if(pwm > PWM_MAX)
        pwm = PWM_MAX;
    
    uint8_t blue, green;
    if(pwm > PWM_MID)
    {
        blue = 255;
        green = 255*(1.0f-((float)(pwm-PWM_MID))/(PWM_MAX-PWM_MID));
    }
    else if(pwm > 0)
    {
        green = 255;
        blue = 255*(pwm-PWM_LOW)/(PWM_MID-PWM_LOW);
    }
    else
    {
        green = 0;
        blue = 0;
    }
    if(!setSpeed(pwmMonitor, pwm, 200))
        return;
    analogWrite(pins.bLPin, blue);
    analogWrite(pins.gLPin, green);
    
    ledcWrite(0, pwm);
        
    char payload[4] = {0,0,0,0};
    if(utoa(pwm, payload, 10) != NULL)
    {
        mqttClient->publish(SPEED_STATE_TOPIC, payload, true, 0);
    }
}

uint8_t Controller::getPwm(uint32_t maxWaitMS)
{
    return getSpeed(pwmMonitor, maxWaitMS);
}

uint8_t Controller::toggleMode()
{
    mode = mode < 2 ? mode + 1 : 0;
    return setMode(mode);
}


uint8_t Controller::setMode(uint8_t mode)
{
    uint8_t vals[] = {PWM_LOW, PWM_MID, PWM_MAX};

    if(mode > 2)
        this->mode = 2;
    else 
        this->mode = mode;
    return vals[mode];
}

bool Controller::isOn()
{
    return _isOn(onMonitor);
}

bool Controller::waterTooLow()
{
    //The water pin is shorted to ground when there is water in the tank
    return digitalRead(pins.waterPin) == HIGH;
}



void lowWaterBlink(TimerHandle_t timer)
{
    static uint8_t i = 1;
    //The timer id stores the pin
    uint32_t pin = (uint32_t)pvTimerGetTimerID(timer);

    if(++i % 2 == 0)
    {
        digitalWrite(pin, LOW);
    }
    else
    {
        digitalWrite(pin, HIGH);
    }

    if(i / 2 >= BLINK_CYCLES)
    {
        i = 1;
        xTimerStop(timer, 0);
    }
}

void Controller::startLowWaterBlink()
{
    static TimerHandle_t timer = xTimerCreate("blink", pdMS_TO_TICKS(BLINK_TIME_MS), pdTRUE, (void*)pins.rLPin, lowWaterBlink);
    digitalWrite(pins.rLPin, HIGH);
    xTimerStart(timer, 0);
}

void Controller::onWaterChanged()
{
    bool tooLow = waterTooLow();
    if(tooLow)
        turnOn(false);
    mqttClient->publish(WATER_STATE_TOPIC, tooLow ? WATER_EMPTY_PAYLOAD : WATER_GOOD_PAYLOAD, true, 0);
}

