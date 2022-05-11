#pragma once

#include "../ValveHandler.hpp"

using namespace std::chrono;
using namespace farmhub::client;

class Drv8801ValveController
    : public ValveController {
public:

    void begin(
        gpio_num_t enablePin,
        gpio_num_t phasePin,
        gpio_num_t faultPin,
        gpio_num_t sleepPin,
        gpio_num_t mode1Pin,
        gpio_num_t mode2Pin,
        gpio_num_t currentPin) {
        Serial.printf("Initializing valve handler on pins enable = %d, phase = %d, fault = %d, sleep = %d, mode1 = %d, mode2 = %d, current = %d\n",
            enablePin, phasePin, faultPin, sleepPin, mode1Pin, mode2Pin, currentPin);

        this->enablePin = enablePin;
        this->phasePin = phasePin;
        this->faultPin = faultPin;
        this->sleepPin = sleepPin;
        this->mode1Pin = mode1Pin;
        this->mode2Pin = mode2Pin;
        this->currentPin = currentPin;

        pinMode(enablePin, OUTPUT);
        pinMode(phasePin, OUTPUT);
        pinMode(faultPin, INPUT);
        pinMode(sleepPin, OUTPUT);
        pinMode(mode1Pin, OUTPUT);
        pinMode(mode2Pin, OUTPUT);
        pinMode(currentPin, INPUT);

        digitalWrite(mode1Pin, HIGH);
        digitalWrite(mode2Pin, HIGH);
    }

    void forward() override {
        drive(HIGH);
    }

    void reverse() override {
        drive(LOW);
    }

    void stop() override {
        digitalWrite(sleepPin, LOW);
        digitalWrite(enablePin, LOW);
    }

private:
    void drive(bool phase) {
        digitalWrite(sleepPin, HIGH);
        digitalWrite(enablePin, HIGH);
        digitalWrite(phasePin, phase);
    }

    gpio_num_t enablePin;
    gpio_num_t phasePin;
    gpio_num_t faultPin;
    gpio_num_t sleepPin;
    gpio_num_t mode1Pin;
    gpio_num_t mode2Pin;
    gpio_num_t currentPin;
};
