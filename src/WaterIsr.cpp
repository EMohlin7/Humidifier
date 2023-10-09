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
    xTaskCreate(waterHandlerTask)
}