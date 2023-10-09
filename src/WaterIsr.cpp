#include "WaterIsr.h"
#include "Controller.h"

struct waterHandlerArgs{
    Controller* controller;
    SemaphoreHandle_t binSem;
};

void waterHandlerTask(void* args)
{
    waterHandlerArgs* a = (waterHandlerArgs*)args;
    Controller* c = a->controller;
    SemaphoreHandle_t binSem = a->binSem;
    free(a);

    for(;;)
    {
        if(xSemaphoreTake(binSem, portMAX_DELAY) == pdTRUE)
        {
            c->onWaterChanged();
        } 
    }
}

void IRAM_ATTR waterIsr(void* binSem)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)binSem, &woken);
}

bool initWater(uint8_t waterPin, Controller* c)
{
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    if(sem == NULL)
        return false;
    waterHandlerArgs* args = (waterHandlerArgs*)malloc(sizeof(waterHandlerArgs));
    if(args == NULL)
        return false;
    *args = {c, sem};
    if(xTaskCreate(waterHandlerTask, "water", configMINIMAL_STACK_SIZE+1024, args, 3, NULL) != pdPASS)
        return false;

    attachInterruptArg(digitalPinToInterrupt(waterPin), waterIsr, sem, CHANGE);
    return true;
}