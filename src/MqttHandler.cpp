#include "MqttHandler.h"
#include "WifiCredServer.h"
#include <WiFi.h>
#include "MqttTopics.h"
#include "MqttPayloads.h"
#include "Controller.h"


#define MQTT_POLL_DELAY_MS 200


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
    QueueHandle_t servAddrsQ = xQueueCreate(2, sizeof(struct mqttConArgs));
   
    WiFiClient wifi;
    MQTTClient* client = (MQTTClient*)args;
    IPAddress ip;
    client->onMessageAdvanced(onMqttMsg);


    struct mqttConArgs servCon;
    waitForAddress:
    Serial.println("Wait for address");
    startCredServer(servAddrsQ);
    xQueueReceive(servAddrsQ, &servCon, portMAX_DELAY);
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
        Serial.println("loop");
        vTaskDelay(pdMS_TO_TICKS(MQTT_POLL_DELAY_MS));
    }
}

void mqttPublishTask(void* q)
{
    mqttPublishTaskArgs* args = (mqttPublishTaskArgs*)q;
    const QueueHandle_t queue = args->q;
    MQTTClient* client = args->client;
    free(args);
    struct mqttPublishArgs msg;
    for(;;)
    {
        if(xQueueReceive(queue, &msg, portMAX_DELAY) == pdFALSE)
            continue;
        if(client->connected())
            client->publish(*(msg.topic), *(msg.payload), msg.retain, msg.qos);
    }
}


void startMqtt(MQTTClient* client)
{
    /*struct mqttTaskArgs* args = (struct mqttTaskArgs*)malloc(sizeof(struct mqttTaskArgs));
    *args = {client, cb};*/
    xTaskCreate(mqttTask, "mqtt", configMINIMAL_STACK_SIZE+1024*2, client, 2, NULL);
}