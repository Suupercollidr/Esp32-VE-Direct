#include "VEDirectParseMessage.h"

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
    if (label == "Checksum")
        return;
        
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
