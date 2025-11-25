/**
 * @file NTCSensor.h
 * @brief Calculates the temperature for a NTC sensor
 * @param powerPin GPIO pin that provides power to the NTC sensor
 * @param readPin GPIO pin where we read the resistance of the NTC sensor
 * 
 * 
 * @version 0.1
 * @date 2025-11-25
 * 
 */

#pragma once
#include <Arduino.h> // For pinMode, digitalWrite, analogRead
#include <optional>

class NTCSensor
{
private:
    uint8_t powerPin;
    uint8_t readPin;
    const int nominalResistance;
    const int nominalTemperature;
    const int beta;
    const int Rref;
    const float measuredOffset;
    const uint8_t samplingRate;

public:
    NTCSensor(
        uint8_t powerPin, uint8_t readPin,
        int nominalResistance = 10000, int nominalTemperature = 25,
        int beta = 3950, int Rref = 9860, float measuredOffset = 5.82,
        uint8_t samplingRate = 5) : powerPin(powerPin), readPin(readPin),
                                    nominalResistance(nominalResistance), nominalTemperature(nominalTemperature),
                                    beta(beta), Rref(Rref), measuredOffset(measuredOffset),
                                    samplingRate(samplingRate)
    {
        // Set up pin modes in the constructor
        pinMode(powerPin, OUTPUT);
        pinMode(readPin, INPUT);
        digitalWrite(powerPin, LOW); // Start with power off
    }

    std::optional<int> temperature()
    {
        digitalWrite(powerPin, HIGH); // Power on the sensor
        int samples = 0;
        for (uint8_t i = 0; i < samplingRate; i++)
        {
            delay(10);
            samples += analogRead(readPin);
        }
        digitalWrite(powerPin, LOW); // Power off the sensor

        float average = samples / samplingRate; // Calculate average ADC value
        average = (4095 / average) - 1.0;       // Calculate NTC resistance
        average = Rref / average;

        // Have Steinhart calculate the temperature for us
        float temperature;
        temperature = average / nominalResistance;
        temperature = log(temperature);
        temperature /= beta;
        temperature += 1.0 / (nominalResistance + 273.15);
        temperature = 1.0 / temperature;
        temperature -= 273.15;            // convert from K to C
        temperature -= measuredOffset;    // adjust for known offset (at 23°C)
        int roundedTemperature = round(temperature); // Round to int (sensor not that exact anyway)

        if (roundedTemperature > -200)  // Return only reasonable temperatures
        {
            return roundedTemperature;
        }
        else
        {
            //logEvent("No reasonable temperature from NTC", "WARNING"); // TODO: get ntc number or whatever in log message
            return std::nullopt;
        }
    }
};
