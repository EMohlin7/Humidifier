#include "Water.h"
#include "Controller.h"

#define S_CHECK_DELAY_MS 10000

struct waterHandlerArgs{
    Controller* controller;
    SemaphoreHandle_t binSem;
};

void IRAM_ATTR waterIsr(void* binSem)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)binSem, &woken);
}

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



//Makes sure the humidifier turns off if the water is to low and the interrupt meant to turn it off missed for some reason
void safetyTask(void* args){
    Controller* con = (Controller*)args;

    for(;;){
        
        if(con->waterTooLow() && con->isOn())
            con->turnOn(false);
        vTaskDelay(pdMS_TO_TICKS(S_CHECK_DELAY_MS));
    }
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

    if(xTaskCreate(safetyTask, "safety", configMINIMAL_STACK_SIZE+100, c, 4, NULL) != pdPASS)
        return false;

    attachInterruptArg(digitalPinToInterrupt(waterPin), waterIsr, sem, CHANGE);
    return true;
}