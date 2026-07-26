#pragma once
#include <Arduino.h>
#include <map>
#include "maputils.h"

struct VEDirectMqttMapping {
    const char* topic;
    const char* veKey;
    float divisor;
};

inline const VEDirectMqttMapping mpptMqttMappings[] = {
    {"embassy/power/mppt/battery/voltage", "V",   1000.0f},
    {"embassy/power/mppt/battery/current", "I",   1000.0f},
    {"embassy/power/mppt/panel/voltage",   "VPV", 1000.0f},
    {"embassy/power/mppt/panel/power",     "PPV", 1.0f},
};

inline const VEDirectMqttMapping inverterMqttMappings[] = {
    {"embassy/power/inverter/battery/voltage", "V",        1000.0f},
    {"embassy/power/inverter/ac/voltage",      "AC_OUT_V", 100.0f},
    {"embassy/power/inverter/ac/current",      "AC_OUT_I", 10.0f},
    {"embassy/power/inverter/ac/power",        "AC_OUT_S", 1.0f},
};