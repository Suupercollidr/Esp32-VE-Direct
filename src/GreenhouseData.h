#pragma once
#include <Arduino.h>
#include <esp_system.h>
#include <WiFi.h>
#include <map>

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

const char *getResetReason(esp_reset_reason_t reason)
{
  //storeDataToNvs("lastState", "getResetReason");
  switch (reason)
  {
  case ESP_RST_UNKNOWN:
    return "Unknown";
  case ESP_RST_POWERON:
    return "Power on";
  case ESP_RST_EXT:
    return "External reset";
  case ESP_RST_SW:
    return "Software reset";
  case ESP_RST_PANIC:
    return "Software panic";
  case ESP_RST_INT_WDT:
    return "Interrupt watchdog";
  case ESP_RST_TASK_WDT:
    return "Task watchdog";
  case ESP_RST_WDT:
    return "Other watchdogs";
  case ESP_RST_DEEPSLEEP:
    return "Deep sleep";
  case ESP_RST_BROWNOUT:
    return "Brownout";
  case ESP_RST_SDIO:
    return "SDIO";
  default:
    return "Not a valid reset reason";
  }
}