#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include "EventLogger.h"

extern EventLogger eventLog;

// Status-bitar för sensorer
struct SensorStatus
{
    unsigned ClimateSensorStatus : 1;
    unsigned SoilSensor1Status : 1;
    unsigned SoilSensor2Status : 1;
    unsigned OneWireDeviceCount : 5; // 0 = fail, 1-31 = success
};

// Strukturen som skickas via ESP-NOW
#pragma pack(push, 1)
struct GreenhouseSensorData
{
    int16_t outdoorTemp;
    int16_t indoorTemp;
    int16_t soilTemp1;
    int16_t soilTemp2;
    uint16_t indoorHumidity;
    uint16_t soilMoisture1;
    uint16_t soilMoisture2;
    uint16_t batteryVoltage;
    SensorStatus status;
    uint8_t checksum;
};
#pragma pack(pop)

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
            Serial.println("Error initializing ESP-NOW");
            return;
        }

        esp_now_register_recv_cb(onDataReceivedStatic);
    }

    GreenhouseSensorData getData() const { return _data; }

    bool hasNewData() const { return _newDataAvailable; }
    void clearNewDataFlag() { _newDataAvailable = false; }
};

// Initiera statisk instanspekare
ESPNowReceiver *ESPNowReceiver::_instance = nullptr;
