#include "Touch.h"
#include "Controller.h"

#define POLL_DELAY_MS 150
#define TOUCH_WAIT_MS 200


bool newTouch(TickType_t* lastTouch)
{
    TickType_t tick = xTaskGetTickCountFromISR();
    bool newTouch = pdTICKS_TO_MS(tick - *lastTouch) > TOUCH_WAIT_MS;
    *lastTouch = tick;
    return newTouch;
}

void IRAM_ATTR powerIsr(void* queue)
{
    static TickType_t lastTick = 0;
    if(touchRead(12) > 10)
        return;
    touch val = touch::power;
    BaseType_t woken = false;
    if(newTouch(&lastTick))
        xQueueSendFromISR((QueueHandle_t)queue, &val, &woken);
    
}
void IRAM_ATTR modeIsr(void* queue)
{
    static TickType_t lastTick = 0;

    if(touchRead(32) > 10)
        return;
    touch val = touch::mode;
    BaseType_t woken = false;
    if(newTouch(&lastTick))
        xQueueSendFromISR((QueueHandle_t)queue, &val, &woken);
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
    struct touchTaskArgs* args = (struct touchTaskArgs*)malloc(sizeof(struct touchTaskArgs));
    QueueHandle_t q = xQueueCreate(5, sizeof(touch));
    if(args == NULL || q == NULL)
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    
    *args = {q, controller};
    if(xTaskCreate(touchTask, "touch", configMINIMAL_STACK_SIZE + 1024, args, 1, NULL) != pdPASS)
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;

    touchAttachInterruptArg(powerPin, powerIsr, q, 10);
    touchAttachInterruptArg(modePin, modeIsr, q, 10);
    return pdPASS;
}

