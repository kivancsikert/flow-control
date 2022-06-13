#pragma once

#include <DallasTemperature.h>
#include <OneWire.h>

#include "../AbstractEnvironmentHandler.hpp"

using namespace farmhub::client;

class SoilSensorHandler
    : public AbstractEnvironmentHandler {

public:
    SoilSensorHandler() = default;

    void begin(gpio_num_t temperaturePin, gpio_num_t moisturePin) {
        Serial.printf("Initializing DS18B20 soil temperature sensor on pin %d\n", temperaturePin);
        oneWire.begin(temperaturePin);

        // locate devices on the bus
        Serial.print("Locating devices...");
        sensors.begin();
        Serial.print("Found ");
        Serial.print(sensors.getDeviceCount(), DEC);
        Serial.println(" devices.");

        // report parasite power requirements
        Serial.print("Parasite power is: ");
        if (sensors.isParasitePowerMode()) {
            Serial.println("ON");
        } else {
            Serial.println("OFF");
        }

        DeviceAddress thermometer;
        if (!sensors.getAddress(thermometer, 0)) {
            Serial.println("Unable to find address for device");
            enabled = false;
            return;
        }

        // show the addresses we found on the bus
        Serial.print("Device 0 Address: ");
        printAddress(thermometer);
        Serial.println();

        Serial.printf("Initializing soil moisture sensor on pin %d\n", moisturePin);
        this->moisturePin = moisturePin;
        pinMode(moisturePin, INPUT);

        enabled = true;
    }

protected:
    void populateTelemetryInternal(JsonObject& json) override {
        populateTemperature(json);
        populateMoisture(json);
    }

    void populateTemperature(JsonObject& json) {
        if (!sensors.requestTemperaturesByIndex(0)) {
            Serial.println("Failed to get temperature from DS18B20 sensor");
            return;
        }
        float temperature = sensors.getTempCByIndex(0);
        if (temperature == DEVICE_DISCONNECTED_C) {
            Serial.println("Failed to get temperature from DS18B20 sensor");
            return;
        }
        json["soilTemperature"] = temperature;
    }

    void populateMoisture(JsonObject& json) {
        uint16_t soilMoistureValue = analogRead(moisturePin);
        Serial.printf("Soil moisture value: %d\n", soilMoistureValue);

        const double run = WaterValue - AirValue;
        const double rise = 100;
        const double delta = soilMoistureValue - AirValue;
        double moisture = (delta * rise) / run;

        json["soilMoisture"] = moisture;
    }

private:
    // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
    OneWire oneWire;

    // Pass our oneWire reference to Dallas Temperature.
    DallasTemperature sensors { &oneWire };

    void printAddress(DeviceAddress deviceAddress) {
        for (uint8_t i = 0; i < 8; i++) {
            // zero pad the address if necessary
            if (deviceAddress[i] < 16)
                Serial.print("0");
            Serial.print(deviceAddress[i], HEX);
        }
    }

    const int AirValue = 8191;
    const int WaterValue = 3800;
    gpio_num_t moisturePin;
};
