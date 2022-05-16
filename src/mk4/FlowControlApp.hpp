#pragma once

#include "../AbstractFlowControlApp.hpp"
#include "Drv8801ValveController.hpp"
#include "SoilTemperatureHandler.hpp"
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

    bool getLedEnabledState() override {
        return LOW;
    }

    gpio_num_t getFlowMeterPin() override {
        return GPIO_NUM_17;
    }

    Drv8801ValveController::Config valve { this };
};

class FlowControlApp : public AbstractFlowControlApp {
public:
    FlowControlApp()
        : AbstractFlowControlApp(deviceConfig, valveStrategy, valveController) {
        telemetryPublisher.registerProvider(environment);
        telemetryPublisher.registerProvider(soilTemperature);
    }

    void beginApp() override {
        AbstractFlowControlApp::beginApp();
        environment.begin();
        soilTemperature.begin(GPIO_NUM_7);
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
    Drv8801ValveController valveController { deviceConfig.valve };
    SoilTemperatureHandler soilTemperature;

    bool open = false;
};
