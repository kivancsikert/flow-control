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

class FlowMeterApp
    : public Application {
public:
    FlowMeterApp()
        : Application("Flow control", VERSION, deviceConfig, config, wifiProvider) {
        telemetryPublisher.registerProvider(flowMeter);
        telemetryPublisher.registerProvider(valve);
        telemetryPublisher.registerProvider(environment);
    }

protected:
    void beginApp() override {
        flowMeter.begin(FLOW_PIN, LED_PIN);
        valve.begin(VALVE_OPEN_PIN, VALVE_CLOSE_PIN);
        environment.begin(DHT_PIN, DHT_TYPE);
    }

private:
    FlowMeterDeviceConfig deviceConfig;
    BlockingWiFiManagerProvider wifiProvider;
    WiFiClient client;

    MeterHandler::Config config;
    MeterHandler flowMeter { tasks(), config };
    ValveHandler valve { mqtt() };
    EnvironmentHandler environment {};
    TelemetryPublisher telemetryPublisher { tasks(), config.publishInterval, mqtt() };
};

FlowMeterApp app;

void setup() {
    app.begin();
}

void loop() {
    app.loop();
}
