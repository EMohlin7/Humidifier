#include "WifiCredServer.h"
#include <ArduinoHttpServer.h>
#include <Preferences.h>
#include <nvs.h>

#define WIFI_POLL_DELAY_MS 300
#define PORT 80
#define SSID "Humidifier"

#define SSID_MAX_LEN 32
#define PASS_MAX_LEN 63
#define IP_MAX_LEN 15
#define PORT_MAX_LEN 5

#define HTTP_BODY_MAX_SIZE 255

#define SSID_FILE_NAME "ssid"
#define PASS_FILE_NAME "wifi_pass"
#define MQTT_IP_FILE_NAME "ip"
#define MQTT_PORT_FILE_NAME "port"

#define WIFI_CONNECT_DELAY_MS 3000

#define NVS_NAME "connectionCreds"


bool parseCreds(char* destination, const char* source, size_t maxLength)
{
    for(int i = 0, destIndex = 0; destIndex < maxLength; ++destIndex)
    {
        char c = source[i];
        if(c == '%')
        {
            char hex[3] = {source[++i], source[++i], 0};
            int val = 0;
            if(sscanf(hex, "%2x", &val) == EOF)
                return false;
            c = val;
        }

        destination[destIndex] = c;
        if(c == 0)
            break;
        ++i;
    }
    destination[maxLength-1];
    return true;
}


/*
    Returns true if the credentials were read successfully, otherwise false.
    Parameters:
        str: The original null terminated string 
        ssid: The array the null terminated ssid will be put in
        lenSsid: The length of the ssid array
        pass: The array that the null terminated password will be put in
        lenPass: The size of the pass array
*/
bool getCreds(char* str, char* ssid, size_t lenSsid, char* pass, size_t lenPass)
{
    char* id = strtok(str, "&");
    if(id == NULL)
        return false;
    
    char* p = strtok(NULL, "&");
    if(p == NULL)
        return false;
    
    char* name = strtok(id, "=");
    name = strtok(NULL, "=");
    if(name == NULL)
        return false;
    
    char* password = strtok(p, "=");
    password= strtok(NULL, "=");
    if(password == NULL)
        return false;

    if(!parseCreds(ssid, name, lenSsid))
        return false;

    return parseCreds(pass, password, lenPass);
}

void initServer(WiFiServer& server, const char* ssid, const char* password, uint16_t port)
{
    WiFi.softAP(ssid, password);
    Serial.println(WiFi.softAPIP());
    server.begin(port);
}

bool trySavedCreds(nvs_handle_t handle, char* ip, uint16_t* port)
{
    size_t ssidLen = SSID_MAX_LEN+1;
    size_t passLen = PASS_MAX_LEN+1;
    size_t ipLen = IP_MAX_LEN+1;
    char ssid[ssidLen];
    char pass[passLen];
    
    //Return instantly if no creds are saved
    if(nvs_get_str(handle, SSID_FILE_NAME, ssid, &ssidLen))
        return false;
    if(nvs_get_str(handle,PASS_FILE_NAME, pass, &passLen))
        return false;
    if(nvs_get_str(handle, MQTT_IP_FILE_NAME, ip, &ipLen))
        return false;
    if(nvs_get_u16(handle, MQTT_PORT_FILE_NAME, port))
        return false;
    WiFi.begin(ssid, pass);
    vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_DELAY_MS));
    return WiFi.status() == WL_CONNECTED;
}

void finishCredSearch(nvs_handle_t handle, QueueHandle_t mqttQ, const char* ip, uint16_t port)
{
    nvs_close(handle);
    struct mqttConArgs ma = {ip, port};
    xQueueSend(mqttQ, &ma, 0);
}

//Returns true if succesfully connected to wifi using the received credentials
bool connectToWifi(char* body, nvs_handle_t nvsHandle)
{
    // +1 to account for null terminator
    char ssid[SSID_MAX_LEN + 1];
    char pass[PASS_MAX_LEN + 1];
    if(getCreds(body, ssid, SSID_MAX_LEN + 1, pass, PASS_MAX_LEN + 1))
    {
        WiFi.begin(ssid, pass);
        vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_DELAY_MS));
        if(WiFi.status() == WL_CONNECTED)
        {
            // Save the wifi credentials in non volatile storage
            nvs_set_str(nvsHandle, SSID_FILE_NAME, ssid);
            nvs_set_str(nvsHandle, PASS_FILE_NAME, pass);
            nvs_commit(nvsHandle);
            
            return true;
        }
    }
    return false;
}

//Returns true if the form was parsed succesfully
bool handleFormMqtt(char* body, char* ip, size_t ipLen, char* port, size_t portLen, nvs_handle_t nvsHandle, uint16_t* parcedPort)
{
    if(getCreds(body, ip, ipLen, port, portLen))
    {
        *parcedPort = atoi(port);
        if(!*parcedPort)
            return false;
        
        nvs_set_str(nvsHandle, MQTT_IP_FILE_NAME, ip);
        nvs_set_u16(nvsHandle, MQTT_PORT_FILE_NAME, *parcedPort);
        nvs_commit(nvsHandle);
        return true;
    }
    return false;
}



bool sendWifiForm()
{
    //Send the wifi form if we are not connected to wifi
    return WiFi.status() != WL_CONNECTED;
}

void serveCredTask(void* args)
{
    serverTaskArgs a = *(serverTaskArgs*)args;
    free(args);

    char mqttIP[IP_MAX_LEN+1]; // including the null terminator
    uint16_t mqttPort;

    start:
    xSemaphoreTake(a.startServerSignal, portMAX_DELAY);
    nvs_handle_t handle = 0;
    esp_err_t nvsOpenErr = nvs_open(NVS_NAME, NVS_READWRITE, &handle);
    //Try to use saved credentials
    if(!nvsOpenErr && trySavedCreds(handle, mqttIP, &mqttPort))
    {
        finishCredSearch(handle, a.mqttQ, mqttIP, mqttPort);
        goto start;
    }

    WiFiServer server = WiFiServer();
    initServer(server, SSID, NULL, PORT);

    for(;;)
    {
        WiFiClient client = server.available(); 
        if(client.available())
        {   
            ArduinoHttpServer::StreamHttpRequest<1024*3> httpReq(client);
            httpReq.readRequest();
            ArduinoHttpServer::Method method = httpReq.getMethod();
            
            if(method == ArduinoHttpServer::Method::Get)
            {

                client.println(sendWifiForm() ? WIFI_FORM : MQTT_FORM);
                client.stop();
            }
            else
            {
                
                const char* bod = httpReq.getBody();
                
                size_t size = strnlen(bod, HTTP_BODY_MAX_SIZE);
                // +1 to account for the null terminator
                char b[size+1];
                strncpy(b, bod, size);
                b[size] = 0;
                client.println("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n");
                client.stop();

                if(sendWifiForm())
                {   
                    connectToWifi(b, handle);
                }
                else
                {
                    // +1 to account for null terminator
                    char port[PORT_MAX_LEN+1];
                    if(handleFormMqtt(b, mqttIP, IP_MAX_LEN+1, port, PORT_MAX_LEN+1, handle, &mqttPort))
                    {
                        finishCredSearch(handle, a.mqttQ, mqttIP, mqttPort);
                        goto start;
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(WIFI_POLL_DELAY_MS));
    }
}

BaseType_t startCredServer(serverTaskArgs* args)
{
    return xTaskCreate(serveCredTask, "cred", configMINIMAL_STACK_SIZE+1024*10, args, 2, NULL);
}

    