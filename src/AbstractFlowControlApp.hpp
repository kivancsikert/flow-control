#pragma once

#include <functional>

#include <Application.hpp>
#include <wifi/WiFiManagerProvider.hpp>

#include "MeterHandler.hpp"
#include "ValveHandler.hpp"
#include "version.h"

using namespace farmhub::client;

class AbstractFlowControlDeviceConfig : public Application::DeviceConfiguration {
public:
    AbstractFlowControlDeviceConfig(const String& defaultModel)
        : Application::DeviceConfiguration("flow-control", defaultModel) {
    }

    virtual gpio_num_t getLedPin() = 0;

    virtual bool getLedEnabledState() = 0;

    virtual gpio_num_t getFlowMeterPin() = 0;

    /**
     * @brief The Q factor for the flow meter.
     */
    double getFlowMeterQFactor() {
        return 5.0f;
    }
};

class FlowControlAppConfig : public Application::AppConfiguration {
public:
    FlowControlAppConfig()
        : Application::AppConfiguration() {
    }

    MeterHandler::Config meter { this };
    Property<seconds> sleepPeriod { this, "sleepPeriod", seconds::zero() };
};

class LedHandler : public BaseSleepListener {
public:
    LedHandler(SleepHandler& sleep)
        : BaseSleepListener(sleep) {
    }

    void begin(gpio_num_t ledPin, bool enabledState) {
        this->ledPin = ledPin;
        this->enabledState = enabledState;
    }

    bool isEnabled() {
        return enabled;
    }

    void setEnabled(bool enabled) {
        this->enabled = enabled;
        digitalWrite(ledPin, enabled ^ !enabledState);
    }

protected:
    void onWake(WakeEvent& event) override {
        // Turn led on when we start
        Serial.printf("Woken by source: %d\n", event.source);
        pinMode(ledPin, OUTPUT);
        setEnabled(true);
    }

    void onDeepSleep(SleepEvent& event) override {
        // Turn off led when we go to sleep
        Serial.printf("Going to sleep, duration: %ld us\n", (long) event.duration.count());
        setEnabled(false);
    }

private:
    gpio_num_t ledPin;
    bool enabledState;
    bool enabled = false;
};

class AbstractFlowControlApp
    : public Application {
public:
    AbstractFlowControlApp(
        AbstractFlowControlDeviceConfig& deviceConfig, ValveController& valveController)
        : Application("Flow control", VERSION, deviceConfig, config, wifiProvider)
        , deviceConfig(deviceConfig)
        , valve(mqtt, events, valveController) {
        telemetryPublisher.registerProvider(flowMeter);
        telemetryPublisher.registerProvider(valve);
    }

protected:
    virtual void beginApp() override {
        led.begin(deviceConfig.getLedPin(), deviceConfig.getLedEnabledState());
        flowMeter.begin(deviceConfig.getFlowMeterPin(), deviceConfig.getFlowMeterQFactor());
        valve.begin();
    }

private:
    void onSleep() {
        if (config.sleepPeriod.get() > seconds::zero()) {
            sleep.deepSleepFor(config.sleepPeriod.get());
        }
    }

    AbstractFlowControlDeviceConfig& deviceConfig;
    WiFiClient client;

    FlowControlAppConfig config;
    MeterHandler flowMeter { tasks, sleep, config.meter, std::bind(&AbstractFlowControlApp::onSleep, this) };

protected:
    BlockingWiFiManagerProvider wifiProvider;
    LedHandler led { sleep };
    ValveHandler valve;
};
