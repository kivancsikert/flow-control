#pragma once

#include <ArduinoJson.h>
#include <Loopable.hpp>
#include <MqttHandler.hpp>
#include <Telemetry.hpp>

#include <list>

using namespace farmhub::client;

class TelemetryHandler
    : public TimedLoopable<void>{
public:
    TelemetryHandler(        TelemetryPublisher& publisher)
        : publisher(publisher) {
    }

    void begin() {
    }

protected:
    void timedLoop() override {
        publisher.publish();
    }

    milliseconds nextLoopAfter() const override {
        // TODO Make this configurable
        return seconds { 5 };
    }

    void defaultValue() override {
    }

private:
    const String topic;
    TelemetryPublisher& publisher;
};
