
/**
 * @brief Logs events to InfluxDB, a CSV file, and prints them to serial
 *
 * @param client InfluxDBClient object where we can send the event
 * @param sdDetectPin (optional) GPIO pin that is low when an SD card is inserted
 * @param logFileName (optional) Name of the file
 *
 */

#pragma once

#include <Arduino.h>
#include <InfluxDbClient.h>
#include <FS.h>

class EventLogger
{
public:
    enum class LogLevel
    {
        INFO,
        WARNING,
        ERROR
    };

    EventLogger(InfluxDBClient &client,
                int8_t sdDetectPin = -1,
                const char *logFileName = "/system.log");

    void log(const String &message,
             LogLevel level = LogLevel::ERROR);

private:
    int8_t sdDetectPin;
    int lastSdDetectState = -1;
    bool sdAvailable = false;
    const char *logFileName;
    InfluxDBClient &influxClient;

    bool checkSDStatus();

    bool logToFile(const char *timestamp,
                   const String &message,
                   LogLevel level);

    bool logToInfluxDB(const char *timestamp,
                       const String &message,
                       LogLevel level);

    const char *levelToString(LogLevel level);
};