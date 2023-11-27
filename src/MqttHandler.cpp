#include "MqttHandler.h"
#include "WifiCredServer.h"
#include <WiFi.h>
#include "MqttTopics.h"
#include "MqttPayloads.h"
#include "Controller.h"


#define MQTT_POLL_DELAY_MS 500

struct mqttTaskArgs{
    MQTTClient* client;
    QueueHandle_t mqttQ;
    SemaphoreHandle_t serverSignal;
};


//This function is used as the callback when a mqtt msg is received
void onMqttMsg(MQTTClient *client, char topic[], char payload[], int length){
    Controller* c = (Controller*)client->ref;
    Serial.print("Incoming: ");
    Serial.print(topic);
    Serial.println(payload);
    
    if(strncmp(topic, POWER_SET_TOPIC, strlen(POWER_SET_TOPIC)) == 0)
    {
        c->turnOn(strncmp(payload, POWER_ON_PAYLOAD, strlen(POWER_ON_PAYLOAD)) == 0);
    }
    else if(strncmp(topic, SPEED_SET_TOPIC, strlen(SPEED_SET_TOPIC)) == 0)
    {
        // Speed is zero if no conversion could be made
        uint8_t speed = atoi(payload);
        if(speed)
            c->setPwm(speed);
    }
}

void initPublish(MQTTClient* client)
{
    //Tell HA that the device is available
    client->publish(AVAILABILITY_TOPIC, PAYLOAD_AVAILABLE, true, 1);
    //client->loop();
    client->publish(HA_DISCOVERY_POWER_TOPIC, HA_DISCOVERY_POWER_PAYLOAD, true, 1);
    //client->loop();
    client->publish(HA_DISCOVERY_SPEED_TOPIC, HA_DISCOVERY_SPEED_PAYLOAD, true, 1);
    //client->loop();
    client->publish(HA_DISCOVERY_WATER_TOPIC, HA_DISCOVERY_WATER_PAYLOAD, true, 1);
    //client->loop();
    Controller* c = (Controller*)client->ref;
    char payload[4] = {0,0,0,0};
    if(utoa(c->getPwm(200), payload, 10) != NULL)
    {
        client->publish(SPEED_STATE_TOPIC, payload, true, 0);
    }
   // client->loop();
    client->publish(POWER_STATE_TOPIC, c->isOn() ? POWER_ON_PAYLOAD : POWER_OFF_PAYLOAD, true, 0);
    //client->loop();
    client->publish(WATER_STATE_TOPIC, c->waterTooLow() ? WATER_EMPTY_PAYLOAD : WATER_GOOD_PAYLOAD, true, 0);
}

void subToTopics(MQTTClient* client)
{
    client->subscribe(POWER_SET_TOPIC, 0);
    //client->loop();
    client->subscribe(SPEED_SET_TOPIC, 0);
    //client->loop();
}

void mqttTask(void* args)
{   
    mqttTaskArgs a = *(mqttTaskArgs*)args;
    free(args);
    WiFiClient wifi;
    MQTTClient* client = a.client;
    IPAddress ip;
    client->onMessageAdvanced(onMqttMsg);


    struct mqttConArgs servCon;
    waitForAddress:
    Serial.println("Wait for address");
    xSemaphoreGive(a.serverSignal);
    xQueueReceive(a.mqttQ, &servCon, portMAX_DELAY);
    ip.fromString(servCon.ip);
    Serial.println(ip.toString());
    //client->disconnect();
    client->begin(ip, servCon.port, wifi);
    client->setWill(AVAILABILITY_TOPIC, PAYLOAD_NOT_AVAILABLE, true, 1);
    Serial.println(servCon.port);
    
    Serial.print("\nconnecting...");
    for(;;)
    {
        if(WiFi.status() != WL_CONNECTED){
            Serial.println("Wifi Not connected");
            goto waitForAddress;
        }
        if(!client->connected())
        {
            //Give it time to connect to the broker
            for(int i = 0; i < 5; ++i){
                if(client->connect("Humidifier"))
                    break;
                Serial.print(".");
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            if(!client->connected())
                goto waitForAddress;
            
            Serial.println("Connected");
            subToTopics(client);
            initPublish(client);
        }
        client->loop();
        vTaskDelay(pdMS_TO_TICKS(MQTT_POLL_DELAY_MS));
    }
}

BaseType_t startMqtt(MQTTClient* client)
{
    struct serverTaskArgs* serverArgs = (struct serverTaskArgs*)malloc(sizeof(struct serverTaskArgs));
    struct mqttTaskArgs* mqttArgs = (struct mqttTaskArgs*)malloc(sizeof(struct mqttTaskArgs));
    QueueHandle_t mqttQ = xQueueCreate(2, sizeof(struct mqttConArgs));
    SemaphoreHandle_t serverSignal = xSemaphoreCreateBinary();
    if(serverArgs == NULL || mqttQ == NULL || serverSignal == NULL || mqttArgs == NULL)
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    
    *serverArgs = {mqttQ, serverSignal};
    *mqttArgs = {client, mqttQ, serverSignal};
    if(startCredServer(serverArgs) != pdPASS)
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    return xTaskCreate(mqttTask, "mqtt", configMINIMAL_STACK_SIZE+1024*2, mqttArgs, 2, NULL);
}