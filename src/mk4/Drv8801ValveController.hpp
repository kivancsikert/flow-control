#pragma once

#include <cmath>

#include "../ValveHandler.hpp"

using namespace std::chrono;
using namespace farmhub::client;

class Drv8801ValveController
    : public ValveController {

private:
    const uint8_t PWM_PHASE = 0;                                  // PWM channel for phase
    const uint8_t PWM_RESOLUTION = 8;                             // 8 bit
    const int PMW_MAX_VALUE = std::pow(2, PWM_RESOLUTION) - 1;    // 2 ^ PWM_RESOLUTION - 1
    const uint32_t PWM_FREQ = 25000;                              // 25kHz

public:
    void begin(
        gpio_num_t enablePin,
        gpio_num_t phasePin,
        gpio_num_t faultPin,
        gpio_num_t sleepPin,
        gpio_num_t mode1Pin,
        gpio_num_t mode2Pin,
        gpio_num_t currentPin) {
        Serial.printf("Initializing DRV8801 valve handler on pins enable = %d, phase = %d, fault = %d, sleep = %d, mode1 = %d, mode2 = %d, current = %d\n",
            enablePin, phasePin, faultPin, sleepPin, mode1Pin, mode2Pin, currentPin);

        this->enablePin = enablePin;
        this->phasePin = phasePin;
        this->faultPin = faultPin;
        this->sleepPin = sleepPin;
        this->mode1Pin = mode1Pin;
        this->mode2Pin = mode2Pin;
        this->currentPin = currentPin;

        pinMode(enablePin, OUTPUT);
        pinMode(sleepPin, OUTPUT);
        pinMode(mode1Pin, OUTPUT);
        pinMode(mode2Pin, OUTPUT);
        pinMode(phasePin, OUTPUT);
        ledcAttachPin(phasePin, PWM_PHASE);
        ledcSetup(PWM_PHASE, PWM_FREQ, PWM_RESOLUTION);

        pinMode(faultPin, INPUT);
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

        int switchDuty = phase ? PMW_MAX_VALUE : 0;
        int holdDuty = PMW_MAX_VALUE / 2 + (phase ? 1 : -1) * (int) (PMW_MAX_VALUE / 2 * 0.40);
        Serial.printf("Switching with duty = %d, hold = %d\n", switchDuty, holdDuty);
        ledcWrite(PWM_PHASE, switchDuty);
        delay(500);
        ledcWrite(PWM_PHASE, holdDuty);
    }

    gpio_num_t enablePin;
    gpio_num_t phasePin;
    gpio_num_t faultPin;
    gpio_num_t sleepPin;
    gpio_num_t mode1Pin;
    gpio_num_t mode2Pin;
    gpio_num_t currentPin;
};
