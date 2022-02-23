#pragma once

#include <Task.hpp>
#include <Telemetry.hpp>

#include "ValveHandler.hpp"

using namespace farmhub::client;

/**
 * @brief Handles the physical "mode" switch that can manually force the valve to open or close in an emergency.
 */
class ModeHandler
    : public BaseTask,
      public TelemetryProvider {
public:
    enum class Mode {
        /**
         * @brief The mode handler has just been initialized.
         */
        INITIALIZED = -100,

        /**
         * @brief The valve is forced closed, remote commands are ignored.
         */
        CLOSED = -1,

        /**
         * @brief The valve is handled via remote commands.
         */
        AUTO = 0,

        /**
         * @brief The valve is forced open, remote commands are ignored.
         */
        OPEN = 1
    };

    ModeHandler(TaskContainer& tasks, ValveHandler& valveHandler)
        : BaseTask(tasks, "Mode handler")
        , valveHandler(valveHandler) {
    }

    void begin(gpio_num_t openPin, gpio_num_t autoPin, gpio_num_t closePin) {
        this->openPin = openPin;
        pinMode(openPin, INPUT_PULLUP);
        this->autoPin = autoPin;
        pinMode(autoPin, INPUT_PULLUP);
        this->closePin = closePin;
        pinMode(closePin, INPUT_PULLUP);
        Serial.printf("Initializing mode handler on pins open = %d, auto = %d, close = %d\n", openPin, autoPin, closePin);
    }

    void populateTelemetry(JsonObject& json) override {
        json["mode"] = static_cast<int>(mode);
    }

protected:
    const Schedule loop(const Timing& timing) override {
        Mode currentMode;
        if (digitalRead(openPin) == LOW) {
            currentMode = Mode::OPEN;
        } else if (digitalRead(closePin) == LOW) {
            currentMode = Mode::CLOSED;
        } else if (digitalRead(autoPin) == LOW) {
            currentMode = Mode::AUTO;
        } else {
            currentMode = mode;
            Serial.println("Unknown mode, none of the switch pins is LOW");
        }
        if (currentMode != mode) {
            Serial.printf("Mode is now %d (was %d)\n",
                static_cast<int>(currentMode),
                static_cast<int>(mode));
            mode = currentMode;
            switch (mode) {
                case Mode::OPEN:
                    valveHandler.setState(ValveHandler::State::OPEN);
                    break;
                case Mode::CLOSED:
                    valveHandler.setState(ValveHandler::State::CLOSED);
                    break;
                case Mode::AUTO:
                    // Do nothing, it will be handled by the valve handler
                    break;
                default:
                    // We shouldn't be here
                    break;
            }
        }
        return sleepFor(seconds { 1 });
    }

private:
    ValveHandler& valveHandler;

    gpio_num_t openPin;
    gpio_num_t autoPin;
    gpio_num_t closePin;

    Mode mode = Mode::INITIALIZED;
};