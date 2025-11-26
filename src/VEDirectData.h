/**
 * @file VEDirectData.h
 * @brief Fetches the last message in the serial buffer from a Victron VE.Direct device,
 * then parses and stores the data into a map
 * @param HardwareSerial The serial port where the device is connected
 * @param prefix A unique prefix that separates the data from the device from that from other devices
 * @param dataMap The map where the values are stored
 *
 * @version 0.1
 * @date 2025-11-25
 *
 */

#pragma once
#include <HardwareSerial.h>
#include <map>

class VEDirectSerial
{
private:
    HardwareSerial &serial;
    String prefix;
    std::map<String, int> &dataMap;

    /**
     * @brief Get entire serial buffer from Serial object
     *
     * @return String
     */
    String getFromSerial()
    {
        String message;
        while (serial.available() > 0)
        {
            char c = serial.read();
            message += c;
            yield();
        }
        return message;
    }

    String trimMessage(String message)
    {
        int startOfLastBlock = message.lastIndexOf("\r\nPID");
        int endOfLastBlock = message.lastIndexOf("Checksum") + 10;

        if (startOfLastBlock == -1 || endOfLastBlock == -1 || startOfLastBlock > endOfLastBlock)
        {
            String logMsg = "Serial block missing either \"PID\" or \"Checksum\"\r\nMessage block:\r\n" + message;
            // logEvent(logMsg);
            return "";
        }
        return message.substring(startOfLastBlock, endOfLastBlock);
    }
    bool calcChecksum(String message)
    {
        uint8_t checksum = 0;
        for (char c : message)
        {
            checksum = (checksum + int(c)) & 256;   // Nuvarande. Enl. ChatGPT är det fel och returnerar alltid 0
            // checksum = (checksum + (uint8_t)c) % 256;  // Den här ska vara bättre (eller "rätt" rent av)
            // checksum += (uint8_t)c; // Och den här smidigare, p.g.a. automatisk wrap-around vid 256
        }
        return (checksum == 0);
    }

    /**
     * @brief Add each character in the message to a label or value,
     *        except if one of these special characters:
     *
     * Carriage return (0x0D): End of field, finalize and save
     * Line feed       (0x0A): Ignore
     * Tab             (0x09): End of the label, start of the value
     *
     * @param message
     *
     * @todo Should this save to dataMap or is there a better way?
     */
    void storeMessage(String message)
    {
        String fieldLabel;
        String fieldValue;
        bool isFieldValue = false; // True = value, False = label

        for (char c : message)
        {
            switch (c)
            {
            case 0x0D:
                if (!prefix.isEmpty()) // If a prefix was sent to this function, add it to the field label
                {
                    fieldLabel = "VE_" + prefix + "_" + fieldLabel;
                }

                dataMap[fieldLabel] = fieldValue.toInt();

                fieldLabel.clear();
                fieldValue.clear();
                isFieldValue = false;
                break;

            case 0x0A:
                break;

            case 0x09:
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

public:
    VEDirectSerial(HardwareSerial &serial, String prefix, std::map<String, int> &dataMap)
        : serial(serial), prefix(prefix), dataMap(dataMap) {}

    bool update()
    {
        String message = getFromSerial();
        message = trimMessage(message);

        if (!calcChecksum(message))
        {
            String logMsg = "Checksum didn't match\r\nMessage block:\r\n" + message;
            //logEvent(logMsg);
            return false;
        }

        storeMessage(message);
        return true;
    }
};
