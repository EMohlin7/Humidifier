#pragma once
#include <Arduino.h>


enum touch {none = 0, power = 1, mode = 2};
void touchTask(void* receiverQ);

class Controller;
BaseType_t initTouch(Controller* controller, uint8_t powerPin, uint8_t modePin);
