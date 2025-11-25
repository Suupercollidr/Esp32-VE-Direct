#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

class SunTimes
{
private:
    float latitude;
    float longitude;
    String timezone;
    time_t sunriseTime;
    time_t sunsetTime;

    time_t parseISO8601(const char *isoString)
    {
        struct tm tm = {};
        strptime(isoString, "%Y-%m-%dT%H:%M:%S%z", &tm);
        return mktime(&tm); // tolkar tm som lokal tid
    }

public:
    SunTimes(float lat, float lon, const String& tzid = "Europe/Stockholm")
        : latitude(lat), longitude(lon), timezone(tzid), sunriseTime(0), sunsetTime(0) {}

    bool updateSunTimes()
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        String url = String("https://api.sunrise-sunset.org/json?lat=") +
                     latitude + "&lng=" + longitude + "&formatted=0&tzid=" + timezone;

        HTTPClient http;
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode != 200)
        {
            http.end();
            return false;
        }

        String payload = http.getString();
        http.end();

        StaticJsonDocument<1024> doc;
        if (deserializeJson(doc, payload) != DeserializationError::Ok)
            return false;

        sunriseTime = parseISO8601(doc["results"]["sunrise"]);
        sunsetTime = parseISO8601(doc["results"]["sunset"]);
        return true;
    }

    bool isSunUp()
    {
        time_t now = time(nullptr);
        return now >= sunriseTime && now < sunsetTime;
    }

    int16_t minutesSinceSunrise()
    {
        time_t now = time(nullptr);
        return (int16_t)((now - sunriseTime) / 60); // negativ om före uppgång
    }

    int16_t minutesSinceSunset()
    {
        time_t now = time(nullptr);
        return (int16_t)((now - sunsetTime) / 60); // negativ om före nedgång
    }

    time_t getSunriseTime() { return sunriseTime; }
    time_t getSunsetTime() { return sunsetTime; }
};
