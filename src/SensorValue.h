#pragma once
#include <Arduino.h>

struct SensorValue
{
    String strValue;
    bool isNumeric;
    int intValue;

    SensorValue() : strValue(""), isNumeric(false), intValue(0) {}
};