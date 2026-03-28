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

VEDirectParseMessage::VEDirectParseMessage()
{
}

void VEDirectParseMessage::parseMessage(const String &message)
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
    if (isInt(value))
        numData[label] = value.toInt();
    else
        txtData[label] = value;
}

bool VEDirectParseMessage::isInt(String value)
{
    if (value.isEmpty())
        return false;

    for (int i = 0; i < value.length(); i++)
    {
        if (i == 0 && value[i] == '-')
            continue;
        if (!isdigit(value[i]))
            return false;
    }
    return true;
}

bool VEDirectParseMessage::stringToMap(String message)
{
    txtData.clear();
    numData.clear();
    parseMessage(message);

    if (numData.empty() && txtData.empty())
        return false;

    return true;
}

std::map<String, int> VEDirectParseMessage::getIntMap() const
{
    return numData;
}

std::map<String, String> VEDirectParseMessage::getStringMap() const
{
    return txtData;
}
