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

class VEDirectSensor {
private:
    HardwareSerial &serial;
    String prefix;
    std::map<String, int> &dataMap;

    String getFromSerial();
    String trimMessage(String message);
    bool calcChecksum(String message);
    void storeMessage(String message);

public:
    VEDirectSensor(HardwareSerial &serial, String prefix, std::map<String, int> &dataMap)
        : serial(serial), prefix(prefix), dataMap(dataMap) {}

    void update() {
        String message = getFromSerial();
        message = trimMessage(message);
        if (calcChecksum(message)) {
            storeMessage(message);
        }
    }
};
