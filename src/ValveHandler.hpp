#pragma once

#include <Task.hpp>

using namespace std::chrono;
using namespace farmhub::client;

class ValveHandler
    : public BaseTask {
public:
    ValveHandler(TaskContainer& tasks)
        : BaseTask(tasks, "ValveHandler") {
    }

    void begin(gpio_num_t openPin, gpio_num_t closePin) {
        this->openPin = openPin;
        this->closePin = closePin;
        pinMode(openPin, OUTPUT);
        pinMode(closePin, OUTPUT);
    }

    const Schedule loop(const Timing& timing) override {
        if (!open) {
            Serial.println("Opening");
            digitalWrite(openPin, LOW);
            digitalWrite(closePin, HIGH);
            open = true;
        } else {
            Serial.println("Closing");
            digitalWrite(openPin, HIGH);
            digitalWrite(closePin, LOW);
            open = false;
        }
        delay(250);
        digitalWrite(openPin, HIGH);
        digitalWrite(closePin, HIGH);
        return sleepFor(seconds { 5 });
    }

private:
    gpio_num_t openPin;
    gpio_num_t closePin;

    bool open = true;
};
