#include "VEDirectData.h"
#include "EventLogger.h"

extern EventLogger eventLog;

/**
 * @brief Check if a string represents a valid integer number.
 *
 * Supports optional leading '+' or '-' sign.
 *
 * @param inputString
 * @return true if inputString is numeric
 * @return false otherwise
 */
bool VEDirectSerial::isNumeric(const String &inputString)
{
    if (inputString.length() == 0)
        return false;
    int i = 0;
    if (inputString[0] == '-' || inputString[0] == '+')
        i++;
    for (; i < inputString.length(); i++)
    {
        if (!isDigit(inputString[i]))
            return false;
    }
    return true;
}

/**
 * @brief Reads all available bytes from the Serial buffer and returns them as a String.
 *
 * @return String containing all currently available bytes in Serial
 */
String VEDirectSerial::getMessageFromSerial()
{
    String message;
    while (serial.available() > 0)
    {
        char c = serial.read();
        message += c;
    }
    return message;
}

/**
 * @brief Get the last message from the serial number by locating the last
 *        occurance of "\r\nPID" (which is the start of a message). Also
 *        verify that the message is complete, by checking that the last
 *        occurance of the Checksum is after the last "\r\nPID".
 *
 * @param message
 * @return String
 */
String VEDirectSerial::trimMessage(String message)
{
    int startOfLastBlock = message.lastIndexOf("\r\nPID");
    int endOfLastBlock = message.lastIndexOf("Checksum") + 10;

    if (startOfLastBlock == -1 || endOfLastBlock == -1 || startOfLastBlock > endOfLastBlock)
    {
        String logMsg = "Serial block missing either \"PID\" or \"Checksum\"\r\nMessage block:\r\n" + message;
        eventLog.log(logMsg, EventLogger::LogLevel::ERROR);
        return "";
    }
    return message.substring(startOfLastBlock, endOfLastBlock);
}


/**
 * @brief Calculate and verify the checksum of a VE.Direct message block.
 *
 *        Each block contains a checksum byte at the end. The sum of all bytes modulo 256
 *        should equal zero for a valid message.
 *
 * @param message The message block to check
 * @return true if checksum is valid
 * @return false if checksum is invalid
 *
 * @note Need to test what calculation really works. ChatGPT doesn't agree with the current implementation.
 */

bool VEDirectSerial::calcChecksum(String message)
{
    uint8_t checksum = 0;
    for (char c : message)
    {
        checksum = (checksum + int(c)) & 256; // Nuvarande. Enl. ChatGPT är det fel och returnerar alltid 0
        // checksum = (checksum + (uint8_t)c) % 256;  // Den här ska vara bättre (eller "rätt" rent av)
        // checksum += (uint8_t)c; // Och den här smidigare, p.g.a. automatisk wrap-around vid 256
    }
    return (checksum == 0);
}

/**
 * @brief Parse a VE.Direct message block and store fields in the internal map.
 *
 *        Each character in the message is appended to a label or value,
 *        except if one of these special characters:
 *
 * Carriage return (0x0D): End of field, finalize and save
 * Line feed       (0x0A): Ignore
 * Tab             (0x09): End of the label, start of the value
 *
 *        Adds prefix to the label, if a prefix exists.
 *        Each field is stored as a SensorValue in sensorData, with both raw string
 *        and integer (if the value is numerical).
 *
 * @param message The trimmed VE.Direct message block
 *
 */

void VEDirectSerial::storeMessage(String message)
{
    String fieldLabel;
    String fieldValue;
    bool isFieldValue = false; // True = value, False = label

    for (char c : message)
    {
        switch (c)
        {
        case 0x0D: // carriage return

            if (!prefix.isEmpty()) // If a prefix was sent to this function, add it to the field label
                fieldLabel = "VE_" + prefix + "_" + fieldLabel;

            if (fieldLabel.length() > 0)
            {
                SensorValue val;
                val.strValue = fieldValue;
                val.isNumeric = isNumeric(fieldValue);
                val.intValue = val.isNumeric ? fieldValue.toInt() : 0;
                sensorData[fieldLabel] = val;
            }

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

/**
 * @brief Read, parse, and store the latest message from the VE.Direct device.
 *
 *        This method reads the Serial buffer, trims it to the last complete message,
 *        verifies the checksum, and stores the values in the internal map.
 *
 * @return true if a valid message was read and stored
 * @return false if checksum failed or message was invalid
 */

bool VEDirectSerial::update()
{
    String message = getMessageFromSerial();
    message = trimMessage(message);

    if (!calcChecksum(message))
    {
        String logMsg = "Checksum didn't match\r\nMessage block:\r\n" + message;
        eventLog.log(logMsg, EventLogger::LogLevel::ERROR);
        return false;
    }

    storeMessage(message);
    return true;
}

/**
 * @brief Access the parsed sensor values.
 *
 * @return const reference to the internal map of field labels to SensorValue
 */
const std::map<String, SensorValue> &VEDirectSerial::getData() const
{
    return sensorData;
}