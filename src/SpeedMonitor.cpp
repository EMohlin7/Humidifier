#include "SpeedMonitor.h"

struct speedMonitor* createSpeedMonitor(uint8_t val)
{
    struct speedMonitor* m = (struct speedMonitor*)malloc(sizeof(struct speedMonitor));
    SemaphoreHandle_t s = xSemaphoreCreateMutex();
    if(s == NULL)
        return NULL;
        
    *m = {s, val};
    return m;
}

uint8_t getSpeed(struct speedMonitor* m, uint32_t maxWaitMS)
{
    if(xSemaphoreTake(m->mutex, pdMS_TO_TICKS(maxWaitMS)) == pdFALSE)
        return 0;
    else
    {
        uint8_t speed;
        speed = m->speed;
        xSemaphoreGive(m->mutex);
        return speed;
    }
}

bool setSpeed(struct speedMonitor* m, uint8_t speed, uint32_t maxWaitMS)
{
    if(xSemaphoreTake(m->mutex, pdMS_TO_TICKS(maxWaitMS)) == pdFALSE)
        return false;
    else
    {
        m->speed = speed;
        xSemaphoreGive(m->mutex);
        return true;
    }
}