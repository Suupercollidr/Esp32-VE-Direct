#pragma once
#include <Arduino.h>
#include <map>
#include "maputils.h"

const std::map<String, String> mpptMqttMappings = {
    {"V", "embassy/power/mppt/battery/voltage"},
    {"I", "embassy/power/mppt/battery/current"},
    {"VPV", "embassy/power/mppt/panel/voltage"},
    {"PPV", "embassy/power/mppt/panel/power"},
    {"H20", "embassy/power/mppt/panel/yield_today"},
    {"H22", "embassy/power/mppt/panel/yield_yesterday"}
};

const std::map<String, String> inverterMqttMappings = {
    {"V", "embassy/power/inverter/battery/voltage"},
    {"AC_OUT_V", "embassy/power/inverter/ac/voltage"},
    {"AC_OUT_I", "embassy/power/inverter/ac/current"},
    {"AC_OUT_S", "embassy/power/inverter/ac/power"}
};