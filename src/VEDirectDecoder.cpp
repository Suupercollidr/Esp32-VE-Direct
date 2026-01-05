#include <algorithm>
#include "VEDirectDecoder.h"
#include "SensorValue.h"


extern EventLogger eventLog;

SensorValue VEDirectDecoder::decodeToSensorValue(const String &label, int value)
{
    SensorValue result;
    result.intValue = value;
    result.isNumeric = true;

    // Hitta vilka koder som ingår
    std::vector<int> codes = findCodes(label, value);

    if (codes.empty())
    {
        eventLog.log("Hittade inga felmeddelanden för " + String(value));
        result.strValue = String(value); // If we can't find anything, keep the numerical value
        return result;
    }

    // Look up message text
    result.strValue = lookupMessages(label, codes);
    return result;
}

std::map<String, SensorValue> VEDirectDecoder::decodeMap(const std::map<String, SensorValue> &rawData)
{
    std::map<String, SensorValue> result;
    
    for (const auto &entry : rawData)
    {
        const String &label = entry.first;
        const SensorValue &val = entry.second;

        if (val.isNumeric && mappings.count(label))
        {
            result[label] = decodeToSensorValue(label, val.intValue);
        }
        else
        {
            result[label] = val; // lämna orört
        }
    }
    return result;
}

/*
 *  Find all error codes hidden in value
 * - exact match
 * - otherwise try to extract combination
 */
std::vector<int> VEDirectDecoder::findCodes(const String &label, int value)
{
    auto it = mappings.find(label);
    if (it == mappings.end())
        return {};

    const std::map<int, String> &codeMap = it->second;

    // Exact match
    if (codeMap.count(value))
        return {value};

    // Combination
    eventLog.log("Trying to find error codes for " + label + " hidden in " + String(value), EventLogger::LogLevel::INFO);
    std::vector<int> availableCodes;
    for (const auto &entry : codeMap)
    {
        availableCodes.push_back(entry.first);
    }

    if (availableCodes.empty())
    {
        eventLog.log("No error codes found for " + label + " (error code " + value + ")");
        return {};
    }
    return findCombination(availableCodes, value);
}

/*
 * Convert codes to human readable string
 */
String VEDirectDecoder::lookupMessages(const String &label,
                                       const std::vector<int> &codes)
{
    auto it = mappings.find(label);
    if (it == mappings.end())
    {
        eventLog.log(String("Unknown label " + label), EventLogger::LogLevel::ERROR);
        return "Okänd label: " + label;
    }
    const std::map<int, String> &codeMap = it->second;
    String result;

    for (size_t i = 0; i < codes.size(); i++)
    {
        auto msgIt = codeMap.find(codes[i]);
        if (msgIt == codeMap.end())
            continue;

        if (i > 0)
            result += ", ";

        result += msgIt->second;
    }

    return result;
}

/*
 * Greedy combination solver
 * Returns empty vector if exact sum not possible
 */
std::vector<int> VEDirectDecoder::findCombination(
    const std::vector<int> &availableCodes,
    int value)
{
    std::vector<int> result;
    int remaining = value;

    // Sort descending
    std::vector<int> sorted = availableCodes;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());

    for (int code : sorted)
    {
        if (code <= remaining)
        {
            result.push_back(code);
            remaining -= code;
        }

        if (remaining == 0)
            break;
    }

    // Not an exact match, fail safely
    if (remaining != 0)
    {
        eventLog.log(String("Failed to find error codes in " + value), EventLogger::LogLevel::WARNING);
        return {};
    }
    return result;
}
