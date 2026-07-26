#pragma once
#include <map>
#include <Arduino.h>

/**
 * @brief Parse a VE.Direct message block and store fields to maps.
 *
 *        Each character in the message is appended to a label or value,
 *        except if one of these special characters:
 *
 * Carriage return (0x0D): End of field, finalize and save
 * Line feed       (0x0A): Ignore
 * Tab             (0x09): End of the label, start of the value
 *
 *        Each field is stored as an int in the map.
 *
 * @param message The trimmed VE.Direct message block
 *
 */

class VEDirectParseMessage
{

private:
    std::map<String, int> numData;
    std::map<String, String> txtData;
    void parseMessage(const String &message);
    void storeField(const String &label, const String &value);
    bool isInt(String value);

public:
    explicit VEDirectParseMessage();

    bool stringToMap(String message);

    std::map<String, int> getIntMap() const;
    std::map<String, String> getStringMap() const;
};
