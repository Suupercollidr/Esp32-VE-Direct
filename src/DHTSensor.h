#pragma once
#include <DHTesp.h>
#include <optional>
#include "logger.h"

class DHTSensor
{
private:
    DHTesp dht;
    uint8_t pin;
    uint32_t lastReading;
    float lastHumidity;
    float lastTemperature;
    bool lastReadValid;

    // Updates readings only if enough time has passed
    void updateReadingsIfNeeded()
    {
        uint32_t minSamplingPeriod = dht.getMinimumSamplingPeriod();
        uint32_t timeSinceLastReading = millis() - lastReading;

        // Only update if enough time has passed
        if (timeSinceLastReading >= minSamplingPeriod || lastReading == 0)
        {
            TempAndHumidity newData = dht.getTempAndHumidity();
            lastHumidity = newData.humidity;
            lastTemperature = newData.temperature;
            lastReading = millis();
        }
    }

public:
    DHTSensor(uint8_t pin)
        : pin(pin), dht(), lastReading(0),
          lastHumidity(0), lastTemperature(0)
    {
        dht.setup(pin, DHTesp::DHT11);
    }

    // Returns temperature in °C (rounded), or std::nullopt if invalid
    std::optional<int> temperature()
    {
        updateReadingsIfNeeded();
        if (lastTemperature < 300)
        {
            return round(lastTemperature);
        }
        else
        {
           // logEvent("Invalid temperature reading from DHT sensor", "WARNING");
            return std::nullopt;
        }
    }
    
    // Returns humidity in %RH (rounded), or std::nullopt if invalid
    std::optional<int> humidity()
    {
        updateReadingsIfNeeded();
        if (lastHumidity < 300)
        {
            return round(lastHumidity);
        }
        else
        {
          //  logEvent("Invalid humidity reading from DHT sensor", "WARNING");
            return std::nullopt;
        }
    }
};
