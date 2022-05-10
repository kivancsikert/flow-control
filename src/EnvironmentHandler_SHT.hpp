#pragma once

#include <Wire.h>

#include <SHTSensor.h>

#include <Telemetry.hpp>

using namespace farmhub::client;

class EnvironmentHandler
    : public TelemetryProvider {

public:
    EnvironmentHandler() = default;

    void begin() {
        Serial.print("Initializing SHT sensor");
        Wire.begin();

        if (sht.init()) {
            // only supported by SHT3x
            sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM);
            enabled = true;
            Serial.print("init(): success\n");
        } else {
            Serial.print("init(): failed\n");
            enabled = false;
        }
    }

protected:
    void populateTelemetry(JsonObject& json) override {
        if (!enabled) {
            return;
        }
        auto temperature = sht.getTemperature();
        auto humidity = sht.getHumidity();
        Serial.printf("Temperature: %f, humidity: %f\n", temperature, humidity);
        json["temperature"] = temperature;
        json["humidity"] = humidity;
    }

private:
    SHTSensor sht;
    bool enabled = false;
};
