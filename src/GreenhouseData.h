#pragma once
#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// Bitfield för sensorstatus
typedef struct
{
    unsigned ClimateSensorStatus : 1;
    unsigned SoilSensor1Status    : 1;
    unsigned SoilSensor2Status    : 1;
    unsigned OneWireDeviceCount   : 5; // 0 = ingen sensor, 1-31 = antal sensorer
} SensorStatus;

// Packad struct för ESP-NOW data
#pragma pack(push, 1)
typedef struct
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
} GreenhouseSensorData;
#pragma pack(pop)

// Checksum-funktion (samma som på B-2)
inline uint8_t calculateChecksum(const GreenhouseSensorData &data)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&data);
    uint8_t sum = 0;

    // Exkludera checksumfältet självt
    for (size_t i = 0; i < sizeof(GreenhouseSensorData) - sizeof(data.checksum); ++i)
        sum ^= bytes[i];

    return sum;
}

#ifdef __cplusplus
}
#endif
