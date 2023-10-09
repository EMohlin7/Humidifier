#pragma once
#include "Modes.h"




#define AVAILABILITY_TOPIC "humidifer/availability"

#define HA_DISCOVERY_POWER_TOPIC "homeassistant/switch/humidifier_switch/config"
#define POWER_STATE_TOPIC  "humidifier/power/state"
#define POWER_SET_TOPIC  "humidifier/power/set"


#define HA_DISCOVERY_SPEED_TOPIC "homeassistant/number/humidifier_speed/config"
#define SPEED_STATE_TOPIC  "humidifier/speed/state"
#define SPEED_SET_TOPIC  "humidifier/speed/set"

#define HA_DISCOVERY_WATER_TOPIC "homeassistant/binary_sensor/humidifier_water/config"
#define WATER_STATE_TOPIC "humidifier/water/state"
