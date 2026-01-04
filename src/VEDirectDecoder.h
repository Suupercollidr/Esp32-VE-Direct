#pragma once
#include <Arduino.h>
#include <map>
#include <vector>
#include "EventLogger.h"
#include "mappings.h"

struct SensorValue {
    String strValue;
    bool isNumeric;
    int intValue;

    SensorValue() : strValue(""), isNumeric(false), intValue(0) {}
};

class VEDirectDecoder {
public:
    VEDirectDecoder(EventLogger &logger);

    // Returnerar text för en kod
    String decode(const String &label, int code);

    // Returnerar SensorValue med strValue och intValue
    SensorValue decodeToSensorValue(const String &label, int code);

    // Registrera eller uppdatera mappings för en label
    void setMapping(const String &label, const std::map<int, String> &codeMap);

private:
    EventLogger &eventLog;
    std::map<String, std::map<int, String>> labelMappings;

    // Iterativ metod för att hitta kombinationer av koder
    std::vector<int> findCombination(const std::vector<int> &availableCodes, int code);
};
