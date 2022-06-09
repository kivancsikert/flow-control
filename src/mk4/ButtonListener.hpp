#pragma once

#include <functional>

#include <Task.hpp>

using namespace farmhub::client;

class ButtonListener : public BaseTask {

public:
    ButtonListener(TaskContainer& tasks, const String& name, gpio_num_t pin, uint8_t mode, microseconds triggerDelay, std::function<void()> trigger)
        : BaseTask(tasks, name)
        , pin(pin)
        , triggerDelay(triggerDelay)
        , trigger(trigger) {
        pinMode(pin, mode);
    }

protected:
    const Schedule loop(const Timing& timing) override {
        if (digitalRead(pin) == LOW) {
            if (!pressed) {
                Serial.println("Button pressed for firs ttime");
                pressed = true;
                pressedSince = timing.loopStartTime;
            } else if (!triggered && timing.loopStartTime - pressedSince > triggerDelay) {
                Serial.println("Button triggering");
                triggered = true;
                trigger();
            }
        } else {
            if (pressed) {
                Serial.println("Button released");
                pressed = false;
                triggered = false;
            }
        }
        return sleepFor(milliseconds { 100 });
    }

private:
    const gpio_num_t pin;
    const microseconds triggerDelay;
    const std::function<void()> trigger;

    bool pressed = false;
    bool triggered = false;
    time_point<boot_clock> pressedSince;
};
