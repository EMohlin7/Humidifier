#pragma once
#include <WiFi.h>
struct serverTaskArgs{
  QueueHandle_t mqttQ;
  SemaphoreHandle_t startServerSignal;
};

struct mqttConArgs{
    const char* ip;
    uint32_t port;
};

BaseType_t startCredServer(serverTaskArgs* args);
const String WIFI_FORM = 
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Accept: application/x-www-form-urlencoded\r\n"
  "Connection: close\r\n\r\n"
  "<!DOCTYPE html>\n"
  "<html>\n"
  "<head>\n"
  "    <title>Login Form</title>\n"
  "</head>\n"
  "<body>\n"
  "    <h2>Login Form</h2>\n"
  "    <form action=\"process_form.php\" method=\"post\">\n"
  "        <label for=\"ssid\">SSID:</label>\n"
  "        <input type=\"text\" id=\"ssid\" name=\"ssid\" required>\n"
  "        <br><br>\n"
  "        <label for=\"password\">Password:</label>\n"
  "        <input type=\"password\" id=\"password\" name=\"password\" required>\n"
  "        <br><br>\n"
  "        <input type=\"submit\" value=\"Submit\">\n"
  "    </form>\n"
  "</body>\n"
  "</html>";

  const String MQTT_FORM = 
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Accept: application/x-www-form-urlencoded\r\n"
  "Connection: close\r\n\r\n"
  "<!DOCTYPE html>\n"
  "<html>\n"
  "<head>\n"
  "    <title>Login Form</title>\n"
  "</head>\n"
  "<body>\n"
  "    <h2>Login Form</h2>\n"
  "    <form action=\"process_form.php\" method=\"post\">\n"
  "        <label for=\"ip\">IP:</label>\n"
  "        <input type=\"text\" id=\"ip\" name=\"ip\" required>\n"
  "        <br><br>\n"
  "        <label for=\"port\">Port:</label>\n"
  "        <input type=\"number\" id=\"port\" name=\"port\" required>\n"
  "        <br><br>\n"
  "        <input type=\"submit\" value=\"Submit\">\n"
  "    </form>\n"
  "</body>\n"
  "</html>";