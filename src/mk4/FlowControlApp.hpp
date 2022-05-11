#pragma once

#include "../AbstractFlowControlApp.hpp"
#include "ShtHandler.hpp"
#include "Drv8801ValveController.hpp"

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
        valveController.begin(
            GPIO_NUM_10,    // Enable
            GPIO_NUM_11,    // Phase
            GPIO_NUM_12,    // Fault
            GPIO_NUM_13,    // Sleep
            GPIO_NUM_14,    // Mode1
            GPIO_NUM_15,    // Mode2
            GPIO_NUM_16     // Current
        );
    }

private:
    FlowControlDeviceConfig deviceConfig;
    ShtHandler environment;
    NormallyClosedValveControlStrategy valveStrategy;
    Drv8801ValveController valveController;
    ValveHandler valve { mqtt, events, valveStrategy, valveController };
    IntervalTask switcher { tasks, "valve-switcher", seconds { 5 }, [this]() {
                               open = !open;
                               valve.setState(open ? ValveHandler::State::OPEN : ValveHandler::State::CLOSED);
                           } };

    bool open = false;
};
