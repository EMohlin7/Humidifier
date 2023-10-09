#pragma once
#include <Arduino.h>
#include <MQTT.h>

struct mqttTaskArgs{
    MQTTClient* client;
    MQTTClientCallbackAdvanced cb;
};



struct mqttPublishTaskArgs{
    MQTTClient* client;
    const QueueHandle_t q;
};

struct mqttPublishArgs{
    String* topic;
    String* payload;
    bool retain;
    int qos;
};

void startMqtt(MQTTClient* client);
void mqttPublishTask(void* q);

