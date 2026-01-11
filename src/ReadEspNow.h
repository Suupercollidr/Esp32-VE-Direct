#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include "EventLogger.h"
#include "GreenhouseData.h"

extern EventLogger eventLog;

// Klass för ESP-NOW mottagning
class ESPNowReceiver
{
private:
    GreenhouseSensorData _data;
    bool _newDataAvailable = false;

    static void onDataReceivedStatic(const uint8_t *mac, const uint8_t *incomingData, int len)
    {
        if (_instance)
            _instance->onDataReceived(mac, incomingData, len);
    }

    void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
    {
        if (len != sizeof(GreenhouseSensorData))
        {
            eventLog.log("Received data has invalid length: " + String(len), EventLogger::LogLevel::WARNING);
            return;
        }

        char macStr[18];
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Serial.printf("Received %d bytes from %s\n", len, macStr);

        memcpy(&_data, incomingData, sizeof(_data));

        if (!verifyChecksum())
        {
            eventLog.log("Recieved data has invalid checksum", EventLogger::LogLevel::WARNING);
            return;
        }
        checkSensorStatus();
        _newDataAvailable = true;
        printData();
    }

    bool verifyChecksum() const
    {
        return calculateChecksum() == _data.checksum;
    }

    uint8_t calculateChecksum() const
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&_data);
        uint8_t sum = 0;
        for (size_t i = 0; i < sizeof(GreenhouseSensorData) - sizeof(_data.checksum); ++i)
            sum ^= bytes[i];
        return sum;
    }

    String toString() const
    {
        String s = "";
        s += "=== Recieved data from ESP-NOW ===\n";
        s += "Indoor Temp: " + String(_data.indoorTemp) + " d°C\n";
        s += "Outdoor Temp: " + String(_data.outdoorTemp) + " d°C\n";
        s += "Soil Temp1: " + String(_data.soilTemp1) + " d°C\n";
        s += "Soil Temp2: " + String(_data.soilTemp2) + " d°C\n";
        s += "Indoor Humidity: " + String(_data.indoorHumidity) + " %RH\n";
        s += "Soil Moisture1: " + String(_data.soilMoisture1) + "\n";
        s += "Soil Moisture2: " + String(_data.soilMoisture2) + "\n";
        s += "Battery Voltage: " + String(_data.batteryVoltage) + " mV\n";
        s += "ClimateSensorStatus: " + String(_data.status.ClimateSensorStatus) + "\n";
        s += "SoilSensor1 status: " + String(_data.status.SoilSensor1Status) + "\n";
        s += "SoilSensor2 status: " + String(_data.status.SoilSensor2Status) + "\n";
        s += "OneWire devices: " + String(_data.status.OneWireDeviceCount) + "\n";
        s += "Checksum: " + String(_data.checksum);
        return s;
    }

    void printData() const
    {
        Serial.println(toString());
    }

    void checkSensorStatus()
    {
        if (_data.status.ClimateSensorStatus)
            eventLog.log("The climate sensor failed to initialize.", EventLogger::LogLevel::WARNING);
        if (_data.status.SoilSensor1Status)
            eventLog.log("Soil sensor 1 failed to initialize.", EventLogger::LogLevel::WARNING);
        if (_data.status.SoilSensor2Status)
            eventLog.log("Soil sensor 2 failed to initialize.", EventLogger::LogLevel::WARNING);
        if (_data.status.OneWireDeviceCount)
            eventLog.log("No OneWire sensors found.", EventLogger::LogLevel::WARNING);
    }

    static ESPNowReceiver *_instance;

public:
    ESPNowReceiver() { _instance = this; }

    void begin()
    {
        WiFi.mode(WIFI_STA);

        if (esp_now_init() != ESP_OK)
        {
            eventLog.log("Error initializing ESP-NOW", EventLogger::LogLevel::ERROR);
            return;
        }
        eventLog.log("ESP-NOW initialized", EventLogger::LogLevel::INFO);

        esp_now_register_recv_cb(onDataReceivedStatic);
    }

    GreenhouseSensorData getData() const { return _data; }

    bool hasNewData() const { return _newDataAvailable; }
    void clearNewDataFlag() { _newDataAvailable = false; }
};

// Initiera statisk instanspekare
ESPNowReceiver *ESPNowReceiver::_instance = nullptr;
