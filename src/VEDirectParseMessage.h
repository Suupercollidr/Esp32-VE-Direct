#pragma once
#include <map>
#include <Arduino.h>

/**
 * @brief Parse a VE.Direct message block and store fields a map.
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
    void parseMessag(const String &message);
    void storeField(const String &label, const String &value);

public:
    explicit VEDirectParseMessage();
    std::map<String, String> parseAsString(String message);
    std::map<String, int> parseAsInt(String message);

    std::map<String, int> getIntData() const;
    std::map<String, String> getStringData() const;
};


VEDirectParseMessage::VEDirectParseMessage()
{
}


void VEDirectParseMessage::parseMessag(const String &message)
{
    String fieldLabel;
    String fieldValue;
    bool isFieldValue = false; // True = value, False = label

    for (char c : message)
    {
        switch (c)
        {
        case 0x0D: // carriage return

            if (fieldLabel.length() > 0)
                storeField(fieldLabel, fieldValue);

            fieldLabel.clear();
            fieldValue.clear();
            isFieldValue = false;
            break;

        case 0x0A: // line feed
            break;

        case 0x09: // tab
            isFieldValue = true;
            break;

        default:
            if (isFieldValue)
                fieldValue += c;
            else
                fieldLabel += c;
            break;
        }
    }
}

void VEDirectParseMessage::storeField(const String &label, const String &value)
{
    txtData[label] = value;

    if (value.length() > 0 && isDigit(value[0])) // If value is numeric, also save it to the int map
        numData[label] = value.toInt();
}

std::map<String,int> VEDirectParseMessage::parseAsInt(String message) {
    txtData.clear();
    numData.clear();
    parseMessag(message);
    return numData;
}

std::map<String,String> VEDirectParseMessage::parseAsString(String message) {
    txtData.clear();
    numData.clear();
    parseMessag(message);
    return txtData;
}

std::map<String, int> VEDirectParseMessage::getIntData() const {
    return numData;
}

std::map<String, String> VEDirectParseMessage::getStringData() const {
    return txtData;
}