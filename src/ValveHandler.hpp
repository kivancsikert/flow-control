#pragma once

#include <Events.hpp>
#include <Task.hpp>
#include <Telemetry.hpp>

#include "ValveScheduler.hpp"

using namespace std::chrono;
using namespace farmhub::client;

RTC_DATA_ATTR
int8_t valveHandlerStoredState;

class ValveController {
public:
    virtual void open() = 0;
    virtual void close() = 0;
    virtual void reset() = 0;
};

/**
 * @brief Handles the valve on an abstract level.
 *
 * Allows opening and closing via {@link ValveHandler#setState}.
 * Handles remote MQTT commands to open and close the valve.
 * Reports the valve's state via MQTT.
 */
class ValveHandler
    : public TelemetryProvider,
      public BaseTask {
public:
    enum class State {
        CLOSED = -1,
        NONE = 0,
        OPEN = 1
    };

    ValveHandler(TaskContainer& tasks, MqttHandler& mqtt, EventHandler& events, ValveController& controller)
        : BaseTask(tasks, "ValveHandler")
        , events(events)
        , controller(controller) {
        mqtt.registerCommand("override", [&](const JsonObject& request, JsonObject& response) {
            State targetState = request["state"].as<State>();
            if (targetState == State::NONE) {
                resume();
            } else {
                seconds duration = request.containsKey("duration")
                    ? request["duration"].as<seconds>()
                    : hours { 1 };
                override(targetState, duration);
                response["duration"] = duration;
            }
            response["state"] = state;
        });
    }

    void populateTelemetry(JsonObject& json) override {
        if (!enabled) {
            return;
        }
        json["valve"] = state;
        if (manualOverrideEnd != time_point<system_clock>()) {
            time_t rawtime = system_clock::to_time_t(manualOverrideEnd);
            auto timeinfo = gmtime(&rawtime);
            char buffer[80];
            strftime(buffer, 80, "%FT%TZ", timeinfo);
            json["overrideEnd"] = string(buffer);
        }
    }

    void begin() {
        controller.reset();

        // RTC memory is reset to 0 upon power-up
        if (valveHandlerStoredState == 0) {
            Serial.println("Initializing for the first time");
        } else {
            Serial.println("Initializing after waking from sleep with state = " + String(valveHandlerStoredState));
            state = valveHandlerStoredState == 1
                ? State::OPEN
                : State::CLOSED;
        }
        enabled = true;
    }

    void setSchedule(const JsonArray schedulesJson) {
        schedules.clear();
        if (schedulesJson.isNull() || schedulesJson.size() == 0) {
            Serial.println("No schedule defined");
        } else {
            Serial.println("Defining schedule:");
            for (JsonVariant scheduleJson : schedulesJson) {
                schedules.emplace_back(scheduleJson.as<JsonObject>());
                Serial.print(" - ");
                serializeJson(scheduleJson, Serial);
                Serial.println();
            }
        }
    }

    void override(State state, seconds duration) {
        Serial.printf("Overriding valve to %d for %d seconds\n", static_cast<int>(state), duration.count());
        manualOverrideEnd = system_clock::now() + duration;
        setState(state);
    }

    void resume() {
        Serial.println("Normal valve operation resumed");
        manualOverrideEnd = time_point<system_clock>();
    }

protected:
    const Schedule loop(const Timing& timing) override {
        if (!enabled || schedules.empty()) {
            return sleepIndefinitely();
        }

        if (manualOverrideEnd < system_clock::now()) {
            if (manualOverrideEnd != time_point<system_clock>()) {
                resume();
            }

            auto targetState = scheduler.isScheduled(schedules, system_clock::now())
                ? State::OPEN
                : State::CLOSED;

            if (state != targetState) {
                switch (targetState) {
                    case State::OPEN:
                        Serial.println("Opening on schedule");
                        break;
                    case State::CLOSED:
                        Serial.println("Closing on schedule");
                        break;
                }
                setState(targetState);
            }
        }

        return sleepFor(seconds { 1 });
    }

private:
    void setState(State state) {
        this->state = state;
        switch (state) {
            case State::OPEN:
                Serial.println("Opening");
                valveHandlerStoredState = 1;
                controller.open();
                break;
            case State::CLOSED:
                Serial.println("Closing");
                valveHandlerStoredState = -1;
                controller.close();
                break;
        }
        events.publishEvent("valve/state", [=](JsonObject& json) {
            json["state"] = state;
        });
    }

    ValveScheduler scheduler;
    EventHandler& events;
    ValveController& controller;

    State state = State::NONE;
    time_point<system_clock> manualOverrideEnd;
    bool enabled = false;
    std::list<ValveSchedule> schedules;
};

bool convertToJson(const ValveHandler::State& src, JsonVariant dst) {
    return dst.set(static_cast<int>(src));
}
void convertFromJson(JsonVariantConst src, ValveHandler::State& dst) {
    dst = static_cast<ValveHandler::State>(src.as<int>());
}
