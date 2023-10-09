#include <Arduino.h>
#include "WifiCredServer.h"
#include "Controller.h"
#include "Touch.h"
#include "power.h"
#include <MQTT.h>
#include "MqttHandler.h"
#include "WaterIsr.h"

#define PWM_PIN 14
#define FAN_PIN 33
#define WATER_PIN 26
#define TOUCH_POWER_PIN 12
#define TOUCH_MODE_PIN 32
#define R_LED_PIN 0
#define G_LED_PIN 0
#define B_LED_PIN 0


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
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
  ledcSetup(0, 30000, 8);
  
  ledcAttachPin(pins->pwmPin, 0);
  struct onMonitor* monitor = createOnMonitor(false);
  MQTTClient* client = new MQTTClient(512);
  Controller* controller = new Controller(pins, monitor, client);
  if(controller == NULL)
  {
    Serial.println("null");
  }

  initWater(pins->waterPin, controller);
  initTouch(controller, pins->pTouchPin, pins->mTouchPin);
  startMqtt(client);
  Serial.println("Setup complete");
}



void loop(){
  vTaskDelete(NULL);
  Serial.println(touchRead(TOUCH_POWER_PIN));
  vTaskDelay(pdMS_TO_TICKS(100));
}

