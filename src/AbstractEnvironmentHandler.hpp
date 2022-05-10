#pragma once

#include <Telemetry.hpp>

using namespace farmhub::client;

class AbstractEnvironmentHandler : public TelemetryProvider {
protected:
    void populateTelemetry(JsonObject& json) override {
        if (!enabled) {
            return;
        }
        populateTelemetryInternal(json);
    }

    virtual void populateTelemetryInternal(JsonObject& json);

    bool enabled = false;
};
