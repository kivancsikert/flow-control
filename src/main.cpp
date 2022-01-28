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
#include "ValveHandler.hpp"
#include "version.h"

using namespace farmhub::client;

const gpio_num_t FLOW_PIN = GPIO_NUM_33;
const gpio_num_t DHT_PIN = GPIO_NUM_26;
const uint8_t DHT_TYPE = DHT11;
const gpio_num_t LED_PIN = GPIO_NUM_19;
const gpio_num_t VALVE_OPEN_PIN = GPIO_NUM_22;
const gpio_num_t VALVE_CLOSE_PIN = GPIO_NUM_25;

class FlowMeterDeviceConfig : public Application::DeviceConfiguration {
public:
    FlowMeterDeviceConfig()
        : Application::DeviceConfiguration("flow-control", "mk2") {
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

class FlowMeterApp
    : public Application {
public:
    FlowMeterApp()
        : Application("Flow control", VERSION, deviceConfig, config, wifiProvider) {
        telemetryPublisher.registerProvider(flowMeter);
        telemetryPublisher.registerProvider(valve);
        telemetryPublisher.registerProvider(environment);

        // Turn led on when we start
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, HIGH);
    }

protected:
    void beginApp() override {
        flowMeter.begin(FLOW_PIN);
        valve.begin(VALVE_OPEN_PIN, VALVE_CLOSE_PIN);
        environment.begin(DHT_PIN, DHT_TYPE);
    }

private:
    void onSleep() {
        if (config.sleepPeriod.get() > seconds::zero()) {
            // Turn off led when we go to sleep
            digitalWrite(LED_PIN, LOW);

            deepSleepFor(config.sleepPeriod.get());
        }
    }

    FlowMeterDeviceConfig deviceConfig;
    BlockingWiFiManagerProvider wifiProvider;
    WiFiClient client;

    FlowMeterAppConfig config;
    MeterHandler flowMeter { tasks, config.meter, std::bind(&FlowMeterApp::onSleep, this) };
    ValveHandler valve { mqtt };
    EnvironmentHandler environment {};
};

FlowMeterApp app;

void setup() {
    app.begin();
}

void loop() {
    app.loop();
}
