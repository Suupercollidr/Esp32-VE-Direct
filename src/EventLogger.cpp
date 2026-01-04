#include "EventLogger.h"
#include <WiFi.h>
#include <SD.h>
#include <time.h>

EventLogger::EventLogger(InfluxDBClient &client,
                         int8_t sdDetectPin,
                         const char *logFileName)

    : influxClient(client),
      sdDetectPin(sdDetectPin),
      logFileName(logFileName)
{
    if (sdDetectPin >= 0)
        pinMode(sdDetectPin, INPUT_PULLUP);
}

void EventLogger::log(const String &message, LogLevel level)
{
    time_t now;
    time(&now);

    char timestamp[30];
    strftime(timestamp, sizeof(timestamp),
             "%Y-%m-%d\t%H:%M:%S", localtime(&now));

    static const char *levelStr = levelToString(level);

    bool fileSuccess = false;
    bool influxSuccess = false;

    if (sdDetectPin >= 0 && digitalRead(sdDetectPin) == LOW)
        fileSuccess = logToFile(timestamp, message, level);

    if (WiFi.status() == WL_CONNECTED)
        influxSuccess = logToInfluxDB(timestamp, message, level);

    Serial.print(timestamp);
    Serial.print("\t");
    Serial.print(levelStr);
    Serial.print("\t");
    Serial.print(message);
    Serial.print(influxSuccess ? " (sent to InfluxDB)" : " (not sent to InfluxDB)");
    if (sdDetectPin >= 0)
        Serial.print(fileSuccess ? " (written to file)" : " (not written to file)");
    Serial.println();
}

bool EventLogger::logToFile(const char *timestamp,
                            const String &message,
                            LogLevel level)
{
    File file = SD.open(logFileName, FILE_APPEND);
    if (!file)
        return false;

    file.print(timestamp);
    file.print(",");
    file.print(levelToString(level));
    file.print(",");
    file.println(message);
    file.close();
    return true;
}

bool EventLogger::logToInfluxDB(const char *timestamp,
                                const String &message,
                                LogLevel level)
{
    Point logPoint("EventLog");
    logPoint.addTag("level", levelToString(level));
    logPoint.addField("message", message);
    logPoint.addField("timestamp", timestamp);

    bool PointSentSuccessfully = influxClient.writePoint(logPoint);

    if (!PointSentSuccessfully)
        Serial.println("InfluxDB error: " + influxClient.getLastErrorMessage());

    return PointSentSuccessfully;
}

const char *EventLogger::levelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARNING:
        return "WARNING";
    case LogLevel::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}