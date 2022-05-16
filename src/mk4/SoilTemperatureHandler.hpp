#pragma once

#include <DallasTemperature.h>
#include <OneWire.h>

#include "../AbstractEnvironmentHandler.hpp"

using namespace farmhub::client;

class SoilTemperatureHandler
    : public AbstractEnvironmentHandler {

public:
    SoilTemperatureHandler() = default;

    void begin(gpio_num_t pin) {
        Serial.printf("Initializing DS18B20 soil temperature sensor on pin %d\n", pin);
        oneWire.begin(pin);

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
        }

        // show the addresses we found on the bus
        Serial.print("Device 0 Address: ");
        printAddress(thermometer);
        Serial.println();

        enabled = true;
    }

protected:
    void populateTelemetryInternal(JsonObject& json) override {
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
};
