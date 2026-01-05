#pragma once
#include <Arduino.h>
#include <map>
#include <vector>
#include "SensorValue.h"
#include "EventLogger.h"

class VEDirectDecoder
{
public:
    using CodeMap  = std::map<int, String>;
    using LabelMap = std::map<String, CodeMap>;

    explicit VEDirectDecoder(const LabelMap &mappings)
        : mappings(mappings) {}

    SensorValue decodeToSensorValue(const String &label, int value);
    std::map<String, SensorValue> decodeMap(const std::map<String, SensorValue> &rawData);

private:
    LabelMap mappings;

    std::vector<int> findCodes(const String &label, int value);
    String lookupMessages(const String &label, const std::vector<int> &codes);
    std::vector<int> findCombination(const std::vector<int> &availableCodes, int value);
};
