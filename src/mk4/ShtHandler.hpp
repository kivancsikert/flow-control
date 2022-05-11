#pragma once

#include <Wire.h>

#include <SHTSensor.h>

#include "../AbstractEnvironmentHandler.hpp"

using namespace farmhub::client;

class ShtHandler
    : public AbstractEnvironmentHandler {

public:
    ShtHandler() = default;

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
    void populateTelemetryInternal(JsonObject& json) override {
        auto temperature = sht.getTemperature();
        auto humidity = sht.getHumidity();
        json["temperature"] = temperature;
        json["humidity"] = humidity;
    }

private:
    SHTSensor sht;
};
