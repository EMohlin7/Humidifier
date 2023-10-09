#include "WifiCredServer.h"
#include <ArduinoHttpServer.h>
#include <Preferences.h>
#include <nvs.h>

#define WIFI_POLL_DELAY_MS 300
#define PORT 80
#define SSID "Humidifier"
#define PASS "wet"

#define SSID_MAX_LEN 32
#define PASS_MAX_LEN 63

#define SSID_FILE_NAME "ssid"
#define PASS_FILE_NAME "wifi_pass"

#define WIFI_CONNECT_DELAY_MS 5000

#define NVS_NAME "connectionCreds"


bool parseCreds(char* destination, const char* source, int maxLength)
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
bool getCreds(char* str, char* ssid, int lenSsid, char* pass, int lenPass)
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
    strncpy(ip, "192.168.1.77", 4+4+4+4);
    *port = 1883;
    size_t ssidLen = SSID_MAX_LEN+1;
    size_t passLen = PASS_MAX_LEN+1;
    char ssid[ssidLen];
    char pass[passLen];
    //Return instantly if no creds are saved
    if(nvs_get_str(handle, SSID_FILE_NAME, ssid, &ssidLen))
        return false;
    if(nvs_get_str(handle,PASS_FILE_NAME, pass, &passLen))
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
    vTaskDelete(NULL);
}

void serveCredTask(void* args)
{
    QueueHandle_t mqttQ = (QueueHandle_t)args;

    nvs_handle_t handle = 0;
    esp_err_t nvsOpenErr = nvs_open(NVS_NAME, NVS_READWRITE, &handle);
 
    char ip[4+4+4+4]; //4+4+4+4 is the max size of an ipv4 address including the null terminator
    uint16_t port;
    
    if(!nvsOpenErr && trySavedCreds(handle, ip, &port))
        finishCredSearch(handle, mqttQ, ip, port);

    WiFiServer server = WiFiServer();
    initServer(server, SSID, NULL, PORT);
    
    for(;;)
    {
        WiFiClient client = server.available(); 
        if(client.available())
        {   
            ArduinoHttpServer::StreamHttpRequest<1024*5> httpReq(client);
            httpReq.readRequest();
            ArduinoHttpServer::Method method = httpReq.getMethod();
            
            if(method == ArduinoHttpServer::Method::Get)
            {
                client.println(res);
                client.stop();
            }
            else
            {
                
                const char* bod = httpReq.getBody();
                // +1 to account for the null terminator
                size_t size = strnlen(bod, 255) + 1;
                char b[size];
                strncpy(b, bod, size-1);
                b[size-1] = 0;
                
                // +1 to account for null terminator
                char ssid[SSID_MAX_LEN + 1];
                char pass[PASS_MAX_LEN + 1];

                if(getCreds(b, ssid, SSID_MAX_LEN+1, pass, PASS_MAX_LEN+1))
                {
                    client.stop();
                    server.stop();
                    
                    WiFi.softAPdisconnect();
                    WiFi.begin(ssid, pass);
                    vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_DELAY_MS));
                    if(WiFi.status() == WL_CONNECTED)
                    {
                        // Save the wifi credentials in non volatile storage
                        if(&nvsOpenErr)
                        {
                            nvs_set_str(handle, SSID_FILE_NAME, ssid);
                            nvs_set_str(handle, PASS_FILE_NAME, pass);
                            nvs_commit(handle);
                        }
                        
                        
                        vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_DELAY_MS));
                        finishCredSearch(handle, mqttQ, "192.168.1.77", 1883);
                    }
                }
            }
            client.stop();
        }
        vTaskDelay(pdMS_TO_TICKS(WIFI_POLL_DELAY_MS));
    }
}

void startCredServer(QueueHandle_t mqttQ)
{
    xTaskCreate(serveCredTask, "cred", configMINIMAL_STACK_SIZE+1024*10, mqttQ, 2, NULL);
}

    