#pragma once
#include <Arduino.h>
#include <esp_system.h>
#include <WiFi.h>
#include <map>
#include <esp_system.h>
#include <esp_log.h>

struct greenhouseSensorData
{
  int16_t outdoorTemp;
  int16_t indoorTemp;
  int16_t soilTemp1;
  int16_t soilTemp2;
  uint16_t indoorHumidity;
  uint16_t soilMoisture1;
  uint16_t soilMoisture2;
  uint16_t batteryVoltage;
  uint8_t checksum;
};



uint8_t calculateChecksum(const greenhouseSensorData &data)
{
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&data);
  uint8_t sum = 0;

  // Exclude the checksum field itself from calculation
  for (size_t i = 0; i < sizeof(greenhouseSensorData) - sizeof(data.checksum); ++i)
  {
    sum ^= bytes[i];
  }
  return sum;
}
