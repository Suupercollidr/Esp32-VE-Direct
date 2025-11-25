#pragma once
#include <HardwareSerial.h>
#include <map>
#include <String>

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
