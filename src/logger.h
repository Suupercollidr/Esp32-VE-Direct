/**
 * @file logger.h
 * @brief Logs events to InfluxDB and to a CSV file on the SD card  
 * @version 0.1
 * @date 2025-11-25
 * 
 */
#pragma once
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <SD.h>
#include <map>

class Logger {
private:
    InfluxDBClient &influxLogClient;
    bool sdInserted;
    const char *logFileName;

    bool logToFile(const char *timestamp, const String &message, const String &level);
    bool logToInfluxDB(const char *timestamp, const String &message, const String &level);

public:
    Logger(
        InfluxDBClient &influxLogClient,
        bool sdInserted, const char *logFileName
    ) : influxLogClient(influxLogClient),
        sdInserted(sdInserted), logFileName(logFileName) {}

    void log(const String &message, const String &level = "ERROR") {
        time_t now;
        time(&now);
        char timestamp[30];
        strftime(timestamp, 30, "%Y-%m-%d\t%H:%M:%S", localtime(&now));

        bool fileSuccess = false;
        bool influxSuccess = false;

        if (sdInserted) {
            fileSuccess = logToFile(timestamp, message, level);
        }
        if (WiFi.status() == WL_CONNECTED) {
            influxSuccess = logToInfluxDB(timestamp, message, level);
        }

        Serial.print(timestamp);
        Serial.print("\t");
        Serial.print(level);
        Serial.print("\t");
        Serial.print(message);
        Serial.print(influxSuccess ? " (sent to InfluxDB)" : " (not sent to InfluxDB)");
        if (sdInserted) {
            Serial.print(fileSuccess ? " (written to file)" : " (not written to file)");
        }
        Serial.println();
    }
};
