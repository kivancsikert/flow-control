#pragma once

#include <Buttons.hpp>

#include "../AbstractFlowControlApp.hpp"
#include "Drv8801ValveController.hpp"
#include "ShtHandler.hpp"
#include "SoilSensorHandler.hpp"

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
        : AbstractFlowControlApp(deviceConfig, valveController) {
        telemetryPublisher.registerProvider(environment);
        telemetryPublisher.registerProvider(soilSensor);
    }

    void beginPeripherials() override {
        resetWifi.begin(GPIO_NUM_0, INPUT_PULLUP);
        environment.begin();
        soilSensor.begin(GPIO_NUM_7, GPIO_NUM_6);
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
    SoilSensorHandler soilSensor;
    Drv8801ValveController valveController { deviceConfig.valve };
    HeldButtonListener resetWifi { tasks, "Reset WIFI", seconds { 5 },
        [&]() {
            Serial.println("Resetting WIFI settings");
            wifiProvider.wm.resetSettings();

            // Blink the LED once for a second
            bool wasEnabled = led.isEnabled();
            led.setEnabled(!wasEnabled);
            delay(1000);
            led.setEnabled(wasEnabled);
        } };

    bool open = false;
};
