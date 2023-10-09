#include "power.h"


struct onMonitor* createOnMonitor(bool defaultVal)
{
    struct onMonitor* monitor = (struct onMonitor*)malloc(sizeof(struct onMonitor));
    *monitor = {defaultVal, xSemaphoreCreateMutex()};
    return monitor;
}

bool _isOn(struct onMonitor* on)
{
    xSemaphoreTake(on->mutex, portMAX_DELAY);
    bool o = on->on;
    xSemaphoreGive(on->mutex);
    return o;
}

void _setOn(struct onMonitor* monitor, bool on)
{
    xSemaphoreTake(monitor->mutex, portMAX_DELAY);
    monitor->on = on; 
    xSemaphoreGive(monitor->mutex);
}
