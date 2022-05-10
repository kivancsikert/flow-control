#pragma once

#include <DHTesp.h>

#include <Telemetry.hpp>

using namespace farmhub::client;

class EnvironmentHandler
    : public TelemetryProvider {

public:
    EnvironmentHandler() = default;

    void begin(gpio_num_t pin, DHTesp::DHT_MODEL_t type) {
        Serial.printf("Initializing DHT sensor type %d on pin %d\n", type, pin);
        dht.setup(pin, type);
        enabled = true;
    }

protected:
    void populateTelemetry(JsonObject& json) override {
        if (!enabled) {
            return;
        }
        auto data = dht.getTempAndHumidity();
        Serial.printf("Temperature: %f, humidity: %f\n", data.temperature, data.humidity);
        json["temperature"] = data.temperature;
        json["humidity"] = data.humidity;
    }

private:
    DHTesp dht;
    bool enabled = false;
};
