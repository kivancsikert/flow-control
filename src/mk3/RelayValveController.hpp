#pragma once

#include "../ValveHandler.hpp"

using namespace std::chrono;
using namespace farmhub::client;

class RelayValveController
    : public ValveController {
public:
    RelayValveController(milliseconds switchDuration)
        : switchDuration(switchDuration) {
    }

    void begin(gpio_num_t openPin, gpio_num_t closePin) {
        Serial.printf("Initializing relay valve controller on pins open = %d, close = %d\n", openPin, closePin);
        this->openPin = openPin;
        this->closePin = closePin;
        pinMode(openPin, OUTPUT);
        pinMode(closePin, OUTPUT);
        reset();
    }

protected:
    void open() override {
        digitalWrite(openPin, LOW);
        digitalWrite(closePin, HIGH);
        delay(switchDuration.count());
        reset();
    }

    void close() override {
        digitalWrite(openPin, HIGH);
        digitalWrite(closePin, LOW);
        delay(switchDuration.count());
        reset();
    }

    void reset() override {
        digitalWrite(openPin, HIGH);
        digitalWrite(closePin, HIGH);
    }

private:
    const milliseconds switchDuration;
    gpio_num_t openPin;
    gpio_num_t closePin;
};
