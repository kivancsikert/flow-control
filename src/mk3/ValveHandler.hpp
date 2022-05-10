#pragma once

#include "../AbstractValveHandler.hpp"

using namespace std::chrono;
using namespace farmhub::client;

class ValveHandler
    : public AbstractValveHandler {
public:
    ValveHandler(MqttHandler& mqtt, EventHandler& events, milliseconds pulseDuration)
        : AbstractValveHandler(mqtt, events, pulseDuration) {
    }

    void begin(gpio_num_t openPin, gpio_num_t closePin) {
        Serial.printf("Initializing valve handler on pins open = %d, close = %d\n", openPin, closePin);
        this->openPin = openPin;
        this->closePin = closePin;
        pinMode(openPin, OUTPUT);
        pinMode(closePin, OUTPUT);

        AbstractValveHandler::begin();
    }

protected:
    void setStateInternal(State state) override {
        switch (state) {
            case State::OPEN:
                digitalWrite(openPin, LOW);
                digitalWrite(closePin, HIGH);
                break;
            case State::CLOSED:
                digitalWrite(openPin, HIGH);
                digitalWrite(closePin, LOW);
                break;
        }
    }

    void resetStateInternal() override {
        digitalWrite(openPin, HIGH);
        digitalWrite(closePin, HIGH);
    }

private:
    gpio_num_t openPin;
    gpio_num_t closePin;
};
