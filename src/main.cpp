#include <Arduino.h>
#include "WifiCredServer.h"
#include "Controller.h"
#include "Touch.h"
#include "power.h"
#include <MQTT.h>
#include "MqttHandler.h"
#include "WaterIsr.h"

#define PWM_PIN 26
#define FAN_PIN 33
#define WATER_PIN 18 
#define TOUCH_POWER_PIN 15
#define TOUCH_MODE_PIN 0
#define R_LED_PIN 2
#define G_LED_PIN 4
#define B_LED_PIN 16


void setupPins(const struct controllerPins* pins)
{
  ledcSetup(0, 30000, 8);
  ledcAttachPin(pins->pwmPin, 0);

  pinMode(pins->fanPin, OUTPUT);
  pinMode(pins->waterPin, INPUT_PULLUP);
  pinMode(pins->rLPin, OUTPUT);
  pinMode(pins->bLPin, OUTPUT);
  pinMode(pins->gLPin, OUTPUT);
}

void setup() {
  // put your setup code here, to run once:
  //Serial.begin(9600);
  
  struct controllerPins* pins = (struct controllerPins*)malloc(sizeof(struct controllerPins));
  *pins = {
    PWM_PIN, //Pwm pin
    FAN_PIN, //Fan pin
    TOUCH_POWER_PIN, //Power touch input pin
    TOUCH_MODE_PIN, //Mode touch input pin
    WATER_PIN, //Water level input pin
    R_LED_PIN, //Status led red
    G_LED_PIN, //Status led green
    B_LED_PIN //Status led blue
  };
  setupPins(pins);

  MQTTClient* client = new MQTTClient(512);
  Controller* controller = new Controller(pins, client);
  if(controller == NULL)
  {
    Serial.println("Failed to initialize");
    return;
  }

  if(!initWater(pins->waterPin, controller) || initTouch(controller, pins->pTouchPin, pins->mTouchPin) != pdPASS || startMqtt(client) != pdPASS)
  {
    Serial.println("Failed to initialize");
    return;
  }
  
  
  Serial.println("Setup complete");
}



void loop(){
  vTaskDelete(NULL);
  Serial.println(touchRead(TOUCH_POWER_PIN));
  vTaskDelay(pdMS_TO_TICKS(100));
}

