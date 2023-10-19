#include "Touch.h"
#include "Controller.h"

#define POLL_DELAY_MS 150
#define TOUCH_WAIT_MS 200
#define TOUCH_THRESHOLD 12


struct isrArgs{
    uint8_t pin;
    touch_value_t threshold;
    QueueHandle_t queue;   
};

bool IRAM_ATTR newTouch(TickType_t* lastTouch)
{
    TickType_t tick = xTaskGetTickCountFromISR();
    bool newTouch = pdTICKS_TO_MS(tick - *lastTouch) > TOUCH_WAIT_MS;
    *lastTouch = tick;
    return newTouch;
}

void IRAM_ATTR handleIsr(struct isrArgs* args, touch type, TickType_t* lastTick)
{
    if(touchRead(args->pin) > args->threshold)
        return;
    BaseType_t woken = false;
    if(newTouch(lastTick))
        xQueueSendFromISR(args->queue, &type, &woken);
}

void IRAM_ATTR powerIsr(void* args)
{
    static TickType_t lastTick = 0;

    handleIsr((struct isrArgs*)args, touch::power, &lastTick);
}

void IRAM_ATTR modeIsr(void* args)
{
    static TickType_t lastTick = 0;

    handleIsr((struct isrArgs*)args, touch::mode, &lastTick);
}



struct touchTaskArgs{
    QueueHandle_t q;
    Controller* controller;
};

void touchTask(void* touchArgs)
{
    struct touchTaskArgs args = *(struct touchTaskArgs*)touchArgs;
    free(touchArgs);
    touch val = touch::none;
    for(;;)
    {
        if(xQueueReceive(args.q, &val, portMAX_DELAY) == pdPASS)
        {
            if(val == touch::power)
                args.controller->turnOn(!args.controller->isOn());
            else if(val == touch::mode && args.controller->isOn())
                args.controller->setPwm(args.controller->toggleMode());
        }
    }
}

BaseType_t initTouch(Controller* controller, uint8_t powerPin, uint8_t modePin)
{
    struct touchTaskArgs* taskArgs = (struct touchTaskArgs*)malloc(sizeof(struct touchTaskArgs));
    struct isrArgs* pIsrArgs = (struct isrArgs*)malloc(2*sizeof(struct isrArgs));
    if(taskArgs == NULL || pIsrArgs == NULL)
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    
    struct isrArgs* mIsrArgs = pIsrArgs+1;
    

    QueueHandle_t q = xQueueCreate(5, sizeof(touch));
    if(taskArgs == NULL || q == NULL)
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    
    *pIsrArgs = {powerPin, TOUCH_THRESHOLD, q};
    *mIsrArgs = {modePin, TOUCH_THRESHOLD, q};
    *taskArgs = {q, controller};
    if(xTaskCreate(touchTask, "touch", configMINIMAL_STACK_SIZE + 1024, taskArgs, 1, NULL) != pdPASS)
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;

    touchAttachInterruptArg(powerPin, powerIsr, pIsrArgs, pIsrArgs->threshold);
    touchAttachInterruptArg(modePin, modeIsr, mIsrArgs, mIsrArgs->threshold);
    return pdPASS;
}

