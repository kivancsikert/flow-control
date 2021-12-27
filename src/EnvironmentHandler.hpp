#pragma once

#include <DHT.h>

#include <Telemetry.hpp>

using namespace farmhub::client;

class EnvironmentHandler
    : public TelemetryProvider {

public:
    EnvironmentHandler() = default;

    void begin(gpio_num_t pin, uint8_t type) {
        Serial.printf("Initializing DHT sensor type %d on pin %d\n", type, pin);
        dht = new DHT(pin, type);
        dht->begin();
    }

protected:
    void populateTelemetry(JsonObject& json) override {
        auto temp = dht->readTemperature();
        auto humidity = dht->readHumidity();
        Serial.printf("Temperature: %f, humidity: %f%\n", temp, humidity);
        json["temperature"] = temp;
        json["humidity"] = humidity;
    }

private:
    DHT* dht;
};
