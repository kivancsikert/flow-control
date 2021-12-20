#pragma once

#include <Telemetry.hpp>

using namespace std::chrono;
using namespace farmhub::client;

RTC_DATA_ATTR
int8_t valveHandlerStoredState;

class ValveHandler
    : public TelemetryProvider {
public:
    enum class State {
        CLOSED,
        OPEN
    };

    ValveHandler(MqttHandler& mqtt) {
        mqtt.registerCommand("set-valve", [&](const JsonObject& request, JsonObject& response) {
            State state = request["state"].as<State>();
            Serial.println("Controlling valve to " + String(static_cast<int>(state)));
            setState(state);
            response["state"] = state;
        });
    }

    void begin(gpio_num_t openPin, gpio_num_t closePin) {
        this->openPin = openPin;
        this->closePin = closePin;
        pinMode(openPin, OUTPUT);
        pinMode(closePin, OUTPUT);
        digitalWrite(openPin, HIGH);
        digitalWrite(closePin, HIGH);

        // RTC memory is reset to 0 upon power-up
        if (valveHandlerStoredState == 0) {
            Serial.println("Initializing for the first time");
            setState(State::CLOSED);
        } else {
            Serial.println("Initializing after waking from sleep with state = " + String(valveHandlerStoredState));
            state = valveHandlerStoredState == 1
                ? State::OPEN
                : State::CLOSED;
        }
    }

    void populateTelemetry(JsonObject& json) override {
        json["valve"] = state;
    }

private:
    void setState(State state) {
        this->state = state;
        switch (state) {
            case State::OPEN:
                Serial.println("Opening");
                valveHandlerStoredState = 1;
                digitalWrite(openPin, LOW);
                digitalWrite(closePin, HIGH);
                break;
            case State::CLOSED:
                Serial.println("Closing");
                valveHandlerStoredState = -1;
                digitalWrite(openPin, HIGH);
                digitalWrite(closePin, LOW);
                break;
        }
        delay(250);
        digitalWrite(openPin, HIGH);
        digitalWrite(closePin, HIGH);
    }

    gpio_num_t openPin;
    gpio_num_t closePin;
    State state;
};

bool convertToJson(const ValveHandler::State& src, JsonVariant dst) {
    return dst.set(static_cast<int>(src));
}
void convertFromJson(JsonVariantConst src, ValveHandler::State& dst) {
    dst = static_cast<ValveHandler::State>(src.as<int>());
}
