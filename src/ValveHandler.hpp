#pragma once

using namespace std::chrono;
using namespace farmhub::client;

class ValveHandler {
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

        // Close on startup
        setState(State::CLOSED);
    }

private:
    void setState(State state) {
        switch (state) {
            case State::OPEN:
                Serial.println("Opening");
                digitalWrite(openPin, LOW);
                digitalWrite(closePin, HIGH);
                break;
            case State::CLOSED:
                Serial.println("Closing");
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
