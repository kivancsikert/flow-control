#pragma once

#include "../AbstractFlowControlApp.hpp"
#include "ShtHandler.hpp"

using namespace farmhub::client;

class FlowControlDeviceConfig : public AbstractFlowControlDeviceConfig {
public:
    FlowControlDeviceConfig()
        : AbstractFlowControlDeviceConfig("mk4") {
    }

    gpio_num_t getLedPin() override {
        return GPIO_NUM_26;
    }

    gpio_num_t getFlowMeterPin() override {
        return GPIO_NUM_17;
    }
};

class FlowControlApp : public AbstractFlowControlApp {
public:
    FlowControlApp()
        : AbstractFlowControlApp(deviceConfig) {
        telemetryPublisher.registerProvider(environment);
    }

    void beginApp() override {
        AbstractFlowControlApp::beginApp();
        environment.begin();
    }

private:
    FlowControlDeviceConfig deviceConfig;
    ShtHandler environment;
};
