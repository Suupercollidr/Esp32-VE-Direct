/**
 * @file VEDirectData.h
 * @brief Reads the latest message from a VE.Direct device via Serial, parses the fields,
 *        and stores them as structured sensor values.
 *
 *        Each field is stored in a map as a SensorValue, containing:
 *        - raw: the original string from the device
 *        - isNumeric: true if the field contains an integer
 *        - intValue: integer value if isNumeric = true, otherwise 0
 *
 *        The class also supports multiple VE.Direct devices by using a unique prefix
 *        for each device's fields.
 *
 * @note The Serial buffer is read as-is. The latest message block is determined by
 *       locating the last occurrence of "\r\nPID" and "Checksum".
 *
 * @version 0.1
 * @date 2025-11-25
 */

#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include <map>

struct SensorValue
{
    String strValue;
    bool isNumeric;
    int intValue;

    // Set default values
    SensorValue() : strValue(""), isNumeric(false), intValue(0) {}
};

class VEDirectSerial
{
private:
    HardwareSerial &serial;
    String prefix;
    std::map<String, SensorValue> sensorData;

    String getMessageFromSerial();
    static String trimMessage(String message);
    bool calcChecksum(String message);
    void storeMessage(String message);
    static bool isNumeric(const String &inputString);

public:
    explicit VEDirectSerial(HardwareSerial &serialPort, const String &prefixName = "")
        : serial(serialPort), prefix(prefixName) {}
    bool update();
    const std::map<String, SensorValue> &getData() const;
};
