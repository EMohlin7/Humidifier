#include "Tasks.h"
#include <Arduino.h>


#define FAN_WAIT_MS 7000



BaseType_t initController(struct controllerPins* pins, QueueHandle_t pwmQ, QueueHandle_t fanQ)
{
    //pinMode(pins->pwmPin, OUTPUT);
    pinMode(pins->fanPin, OUTPUT);
    pinMode(pins->waterPin, INPUT_PULLUP);
    Serial.println(pins->pwmPin);
    Serial.println((uint32_t)pwmQ);
    struct taskArgs* pwmArgs = (struct taskArgs*)malloc(sizeof(struct taskArgs));
    *pwmArgs = {pins->pwmPin, pwmQ};
    struct taskArgs* fanArgs = (struct taskArgs*)malloc(sizeof(struct taskArgs));
    *fanArgs = {pins->fanPin, fanQ};
    
    BaseType_t pwm = xTaskCreate(pwmTask, "Pwm", configMINIMAL_STACK_SIZE + 1024*4, pwmArgs, 0, NULL);
    BaseType_t power = xTaskCreate(fanTask, "fan", configMINIMAL_STACK_SIZE + 1024, fanArgs, 0, NULL);
   

    
    

    if(pwm == pdPASS && power == pdPASS)
        return pdPASS;
    
    return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
}


void pwmTask(void* args){
    
    struct taskArgs* a = (struct taskArgs*)args;
    //free(args);

    for(;;)
    {
        uint8_t currSpeed = 0;
        if(xQueueReceive(a->q, &currSpeed, portMAX_DELAY) == pdPASS)
        {
            Serial.println("PWM task receive");
            //analogWrite(a.pin, currSpeed);
            ledcWrite(0, currSpeed);
        }
        
    }
}

void _stopFan(TimerHandle_t timer)
{
    //The fan pin is stored as the timer id
    uint8_t pin = *(uint8_t*)pvTimerGetTimerID(timer);

    //Only turn of the fan if the system is still turned off
    digitalWrite(pin, HIGH);
}

void fanTask(void* args)
{
    struct taskArgs a = *(struct taskArgs*)args;
    free(args);
    //Store the fan pin as the timer id to use it in the timer callback
    TimerHandle_t fanTimer = xTimerCreate("timer", pdMS_TO_TICKS(FAN_WAIT_MS), pdFALSE, &a.pin, _stopFan);
    for(;;)
    {
        Serial.println("4");
        bool newVal = false;
        if(xQueueReceive(a.q, &newVal, portMAX_DELAY) == pdPASS)
        {
           /*/ //if(newVal == on)
              //  continue;
            on = newVal;
            if(!on)
                xTimerStart(fanTimer, 0);
            else
                digitalWrite(a.pin, HIGH);*/
        }
    }
}


/*void sendFromIsr(QueueHandle_t q, uint8_t val)
{
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(q, &val, &woken);
}

bool IRAM_ATTR newTouch(TickType_t* lastTick)
{
    TickType_t tick = xTaskGetTickCountFromISR();
    bool nt = pdTICKS_TO_MS(tick - *lastTick) > TOUCH_WAIT_MS;
    *lastTick = tick;
    return nt;
}

void IRAM_ATTR modeIsr()
{
    static TickType_t lastTick = 0;
    if(!newTouch(&lastTick))
        return;

    static uint8_t vals[] = {PWM_LOW, PWM_MID, PWM_MAX};
    static uint8_t index = 0;

    index = index < 2 ? index + 1 : 0;
    Serial.println("mode");
    sendFromIsr(pwmQ, vals[index]);
}

void IRAM_ATTR powerIsr()
{
    static TickType_t lastTick = 0;
    
    if(newTouch(&lastTick))
    {
        Serial.println("power");
        sendFromIsr(powerQ, !on);
    }
        
}*/

