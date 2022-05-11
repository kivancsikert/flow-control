#pragma once

#include "../ValveHandler.hpp"

using namespace std::chrono;
using namespace farmhub::client;

class RelayValveController
    : public ValveController {
public:
    void begin(gpio_num_t openPin, gpio_num_t closePin) {
        Serial.printf("Initializing relay valve controller on pins open = %d, close = %d\n", openPin, closePin);
        this->openPin = openPin;
        this->closePin = closePin;
        pinMode(openPin, OUTPUT);
        pinMode(closePin, OUTPUT);
    }

protected:
    void forward() override {
        digitalWrite(openPin, LOW);
        digitalWrite(closePin, HIGH);
    }

    void reverse() override {
        digitalWrite(openPin, HIGH);
        digitalWrite(closePin, LOW);
    }

    void stop() override {
        digitalWrite(openPin, HIGH);
        digitalWrite(closePin, HIGH);
    }

private:
    gpio_num_t openPin;
    gpio_num_t closePin;
};
