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

#include "MeterHandler.hpp"
#include "version.h"

using namespace farmhub::client;

const gpio_num_t FLOW_PIN = GPIO_NUM_33;
const gpio_num_t LED_PIN = GPIO_NUM_19;

class FlowMeterDeviceConfig : public Application::DeviceConfiguration {
public:
    FlowMeterDeviceConfig()
        : Application::DeviceConfiguration("flow-alert", "mk1") {
    }
};

class FlowMeterApp
    : public Application {
public:
    FlowMeterApp()
        : Application("Flow alert", VERSION, deviceConfig, config, wifiProvider) {
        telemetryPublisher.registerProvider(flowMeter);
    }

protected:
    void beginApp() override {
        flowMeter.begin(FLOW_PIN, LED_PIN);
    }

private:
    FlowMeterDeviceConfig deviceConfig;
    BlockingWiFiManagerProvider wifiProvider;
    WiFiClient client;

    MeterHandler::Config config;
    MeterHandler flowMeter { tasks(), config };
    TelemetryPublisher telemetryPublisher { tasks(), config.publishInterval, mqtt() };
};

FlowMeterApp app;

void setup() {
    app.begin();
}

void loop() {
    app.loop();
}
