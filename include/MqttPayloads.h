#pragma once
#include "MqttTopics.h"
#include "Modes.h"

// Number to string
#define NTS(x) STR(x)
#define STR(x) #x

#define HA_DEVICE "{\"identifiers\": [\"humidifier\"], \"name\": \"Humidifier\", \"model\": \"ESP32\"}"
#define PAYLOAD_AVAILABLE "online"
#define PAYLOAD_NOT_AVAILABLE "offline"


#define HA_DISCOVERY_POWER_PAYLOAD "{\"name\": \"Power\", \"state_topic\": \"" POWER_STATE_TOPIC "\", \"command_topic\": \"" POWER_SET_TOPIC "\", \"availability\": [{\"topic\": \"" AVAILABILITY_TOPIC "\"}, {\"topic\": \"" WATER_STATE_TOPIC "\", \"payload_available\": \"" WATER_GOOD_PAYLOAD "\", \"payload_not_available\": \"" WATER_EMPTY_PAYLOAD "\"}], \"unique_id\": \"hum1switch\", \"device\": " HA_DEVICE "}"
#define POWER_ON_PAYLOAD "ON"
#define POWER_OFF_PAYLOAD "OFF"

#define HA_DISCOVERY_SPEED_PAYLOAD "{\"name\": \"Speed\", \"state_topic\": \"" SPEED_STATE_TOPIC "\", \"command_topic\": \"" SPEED_SET_TOPIC "\", \"device_class\": \"power_factor\", \"availability\": [{\"topic\": \"" AVAILABILITY_TOPIC "\"}, {\"topic\": \"" POWER_STATE_TOPIC "\", \"payload_available\": \"" POWER_ON_PAYLOAD "\", \"payload_not_available\": \"" POWER_OFF_PAYLOAD "\"}], \"unique_id\": \"hum1speed\", \"min\": " NTS(PWM_LOW)", \"max\": " NTS(PWM_MAX)", \"device\": " HA_DEVICE "}"

#define HA_DISCOVERY_WATER_PAYLOAD "{\"name\": \"Water\", \"device_class\": \"moisture\", \"state_topic\": \"" WATER_STATE_TOPIC "\", \"availability_topic\": \"" AVAILABILITY_TOPIC "\", \"unique_id\": \"hum1water\", \"device\": " HA_DEVICE "}"
#define WATER_GOOD_PAYLOAD "ON"
#define WATER_EMPTY_PAYLOAD "OFF"