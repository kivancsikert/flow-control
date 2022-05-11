#pragma once

#include <DHTesp.h>

#include "../AbstractEnvironmentHandler.hpp"

using namespace farmhub::client;

class DhtHandler
    : public AbstractEnvironmentHandler {

public:
    DhtHandler() = default;

    void begin(gpio_num_t pin, DHTesp::DHT_MODEL_t type) {
        Serial.printf("Initializing DHT sensor type %d on pin %d\n", type, pin);
        dht.setup(pin, type);
        enabled = true;
    }

protected:
    void populateTelemetryInternal(JsonObject& json) override {
        auto data = dht.getTempAndHumidity();
        json["temperature"] = data.temperature;
        json["humidity"] = data.humidity;
    }

private:
    DHTesp dht;
};
