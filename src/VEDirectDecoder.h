#pragma once
#include <Arduino.h>
#include <map>
#include <vector>
#include "SensorValue.h"
#include "EventLogger.h"
#include "maputils.h"

class VEDirectDecoder
{
public:
    explicit VEDirectDecoder(const std::map<String, String> &displayNames,
                             const CodeMap &mapLabelCodeText)
        : displayNames(displayNames), codesMap(mapLabelCodeText) {}

    String VEDirectCodeToHumanReadable(const String &label, int value);
    std::map<String, String> VEDirectCodeMapToHumanReadable(const std::map<String, int> &rawData);

private:
    std::map<String, String> displayNames;
    CodeMap codesMap;

    std::vector<int> findCodes(const String &label, int value);
    String lookupMessages(const String &label, const std::vector<int> &codes);
    std::vector<int> findCombination(const std::vector<int> &availableCodes, int value);
};
