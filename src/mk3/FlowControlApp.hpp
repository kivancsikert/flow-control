#pragma once

#include "../AbstractFlowControlApp.hpp"
#include "DhtHandler.hpp"
#include "ModeHandler.hpp"
#include "../ValveHandler.hpp"
#include "RelayValveController.hpp"

const gpio_num_t DHT_PIN = GPIO_NUM_26;
const gpio_num_t LED_PIN = GPIO_NUM_19;
const gpio_num_t VALVE_OPEN_PIN = GPIO_NUM_22;
const gpio_num_t VALVE_CLOSE_PIN = GPIO_NUM_25;
const gpio_num_t MODE_OPEN_PIN = GPIO_NUM_5;
const gpio_num_t MODE_AUTO_PIN = GPIO_NUM_23;
const gpio_num_t MODE_CLOSE_PIN = GPIO_NUM_33;

using namespace farmhub::client;

class FlowControlDeviceConfig : public AbstractFlowControlDeviceConfig {
public:
    FlowControlDeviceConfig()
        : AbstractFlowControlDeviceConfig("mk1") {
    }

    gpio_num_t getLedPin() override {
        return LED_PIN;
    }

    gpio_num_t getFlowMeterPin() override {
        if (model.get() == "mk0") {
            return GPIO_NUM_33;
        } else {
            return GPIO_NUM_18;
        }
    }

    DHTesp::DHT_MODEL_t getDhtType() {
        if (model.get() == "mk1") {
            return DHTesp::DHT_MODEL_t::DHT11;
        } else {
            return DHTesp::DHT_MODEL_t::AM2302;
        }
    }

    bool isValvePresent() {
        return model.get() != "mk0";
    }

    /**
     * @brief The amount of time to wait for the latching valve to switch.
     */
    milliseconds getValvePulseDuration() {
        return milliseconds { 250 };
    }

    bool isModeSwitchPresent() {
        return model.get() != "mk0" && model.get() != "mk4";
    }

    bool isEnvironmentSensorPresent() {
        return model.get() != "mk0";
    }
};

class FlowControlApp : public AbstractFlowControlApp {
public:
    FlowControlApp()
        : AbstractFlowControlApp(deviceConfig) {
        telemetryPublisher.registerProvider(environment);
        telemetryPublisher.registerProvider(valve);
        telemetryPublisher.registerProvider(mode);
    }

    void beginApp() override {
        AbstractFlowControlApp::beginApp();

        if (deviceConfig.isValvePresent()) {
            valveController.begin(VALVE_OPEN_PIN, VALVE_CLOSE_PIN);
        }

        if (deviceConfig.isModeSwitchPresent()) {
            mode.begin(MODE_OPEN_PIN, MODE_AUTO_PIN, MODE_CLOSE_PIN);
        }

        if (deviceConfig.isEnvironmentSensorPresent()) {
            environment.begin(DHT_PIN, deviceConfig.getDhtType());
        }
    }

private:
    FlowControlDeviceConfig deviceConfig;
    DhtHandler environment;
    LatchingValveControlStrategy valveStrategy { deviceConfig.getValvePulseDuration()};
    RelayValveController valveController;
    ValveHandler valve { mqtt, events, valveStrategy, valveController };
    ModeHandler mode { tasks, sleep, valve };
};
