#include "VEDirectDecoder.h"
#include <algorithm>
#include "EventLogger.h"

extern EventLogger eventLog;

VEDirectDecoder::VEDirectDecoder() {}

// Registrera en label och dess kod-mapping
void VEDirectDecoder::setMapping(const String &label, const std::map<int, String> &codeMap)
{
    labelMappings[label] = codeMap;
}

// Iterativ, ESP32-vänlig kombinationssökning
std::vector<int> VEDirectDecoder::findCombination(const std::vector<int> &availableCodes, int code)
{
    std::vector<int> combination;
    int remaining = code;

    std::vector<int> sortedCodes = availableCodes;
    std::sort(sortedCodes.rbegin(), sortedCodes.rend()); // sortera fallande

    for (int c : sortedCodes)
    {
        if (c <= remaining)
        {
            combination.push_back(c);
            remaining -= c;
        }
        if (remaining == 0)
            break;
    }

    if (remaining != 0)
        combination.clear(); // ingen exakt kombination hittad
    return combination;
}

// Returnerar text (en kombinerad sträng) för en kod
String VEDirectDecoder::decode(const String &label, int code)
{
    SensorValue val = decodeToSensorValue(label, code);
    return val.strValue;
}

// Returnerar SensorValue (strValue + intValue)
SensorValue VEDirectDecoder::decodeToSensorValue(const String &label, int code)
{
    SensorValue sv;
    sv.intValue = code;
    sv.isNumeric = true;

    try
    {
        if (labelMappings.count(label) == 0)
        {
            sv.strValue = "Okänd label: " + label;
            eventLog.log(sv.strValue, EventLogger::LogLevel::WARNING);
            return sv;
        }

        const auto &mapping = labelMappings[label];

        // Exakt match
        if (mapping.count(code) > 0)
        {
            sv.strValue = mapping.at(code);
            return sv;
        }

        // Kombinationssökning
        std::vector<int> availableCodes;
        for (auto &entry : mapping)
            availableCodes.push_back(entry.first);

        std::vector<int> codesInMessage = findCombination(availableCodes, code);

        if (codesInMessage.empty())
        {
            sv.strValue = "Okänd kombinerad kod: " + String(code);
            eventLog.log(sv.strValue, EventLogger::LogLevel::WARNING);
            return sv;
        }

        // Kombinera alla felmeddelanden
        String result;
        for (size_t i = 0; i < codesInMessage.size(); i++)
        {
            if (i > 0)
                result += ", ";
            result += mapping.at(codesInMessage[i]);
        }
        sv.strValue = result;
        return sv;
    }
    catch (const std::exception &e)
    {
        sv.strValue = "Kunde inte tolka kod: " + String(code);
        eventLog.log("Exception i decodeToSensorValue: " + String(e.what()), EventLogger::LogLevel::ERROR);
        return sv;
    }
}
