#include <Arduino.h>
#include <ArduinoJson.hpp>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <chrono>

#include <Application.hpp>
#include <Task.hpp>
#include <Telemetry.hpp>
#include <wifi/WiFiManagerProvider.hpp>

#include "EnvironmentHandler.hpp"
#include "MeterHandler.hpp"
#include "ModeHandler.hpp"
#include "ValveHandler.hpp"
#include "version.h"

using namespace farmhub::client;

const gpio_num_t FLOW_PIN = GPIO_NUM_18;
const gpio_num_t DHT_PIN = GPIO_NUM_26;
const gpio_num_t LED_PIN = GPIO_NUM_19;
const gpio_num_t VALVE_OPEN_PIN = GPIO_NUM_22;
const gpio_num_t VALVE_CLOSE_PIN = GPIO_NUM_25;
const gpio_num_t MODE_OPEN_PIN = GPIO_NUM_5;
const gpio_num_t MODE_AUTO_PIN = GPIO_NUM_23;
const gpio_num_t MODE_CLOSE_PIN = GPIO_NUM_33;

class FlowMeterDeviceConfig : public Application::DeviceConfiguration {
public:
    FlowMeterDeviceConfig()
        : Application::DeviceConfiguration("flow-control", "mk1") {

    }

    uint8_t getDhtType() {
        if (model.get() == "mk1") {
            return DHT11;
        } else {
            return DHT22;
        }
    }
};

class FlowMeterAppConfig : public Application::AppConfiguration {
public:
    FlowMeterAppConfig()
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

protected:
    void onWake(WakeEvent& event) override {
        // Turn led on when we start
        Serial.printf("Woken by source: %d\n", event.source);
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, HIGH);
    }

    void onDeepSleep(SleepEvent& event) override {
        Serial.println("Turn off led");
        digitalWrite(LED_PIN, LOW);
    }
};

class FlowMeterApp
    : public Application {
public:
    FlowMeterApp()
        : Application("Flow control", VERSION, deviceConfig, config, wifiProvider) {
        telemetryPublisher.registerProvider(flowMeter);
        telemetryPublisher.registerProvider(valve);
        telemetryPublisher.registerProvider(mode);
        telemetryPublisher.registerProvider(environment);
    }

protected:
    void beginApp() override {
        flowMeter.begin(FLOW_PIN);
        valve.begin(VALVE_OPEN_PIN, VALVE_CLOSE_PIN);
        mode.begin(MODE_OPEN_PIN, MODE_AUTO_PIN, MODE_CLOSE_PIN);
        environment.begin(DHT_PIN, deviceConfig.getDhtType());
    }

private:
    void onSleep() {
        if (config.sleepPeriod.get() > seconds::zero()) {
            sleep.deepSleepFor(config.sleepPeriod.get());
        }
    }

    FlowMeterDeviceConfig deviceConfig;
    BlockingWiFiManagerProvider wifiProvider;
    WiFiClient client;

    FlowMeterAppConfig config;
    MeterHandler flowMeter { tasks, sleep, config.meter, std::bind(&FlowMeterApp::onSleep, this) };
    ValveHandler valve { mqtt };
    ModeHandler mode { tasks, valve };
    LedHandler led { sleep };
    EnvironmentHandler environment {};
};

FlowMeterApp app;

void setup() {
    app.begin();
}

void loop() {
    app.loop();
}
