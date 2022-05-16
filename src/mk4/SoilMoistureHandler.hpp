#pragma once

#include "../AbstractEnvironmentHandler.hpp"

using namespace farmhub::client;

class SoilMoistureHandler
    : public AbstractEnvironmentHandler {

public:
    SoilMoistureHandler() = default;

    void begin(gpio_num_t pin) {
        this->pin = pin;

        Serial.printf("Initializing soil moisture sensor on pin %d\n", pin);
        pinMode(pin, INPUT);
        enabled = true;
    }

protected:
    void populateTelemetryInternal(JsonObject& json) override {
        uint16_t soilMoistureValue = analogRead(pin);
        Serial.printf("Soil moisture value: %d\n", soilMoistureValue);

        const double run = WaterValue - AirValue;
        const double rise = 100;
        const double delta = soilMoistureValue - AirValue;
        double moisture = (delta * rise) / run;

        json["soilMoisture"] = moisture;
    }

private:
    const int AirValue = 8191;
    const int WaterValue = 3800;
    gpio_num_t pin;
};
