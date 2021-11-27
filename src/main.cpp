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

using namespace std::chrono;
using namespace farmhub::client;

const gpio_num_t FLOW_PIN = GPIO_NUM_33;
const gpio_num_t LED_PIN = GPIO_NUM_19;

class FlowMeterDeviceConfig : public Application::DeviceConfiguration {
public:
    FlowMeterDeviceConfig()
        : Application::DeviceConfiguration("flow-alert", "mk1") {
    }
};

class TelemetryTask
    : public Task {
public:
    TelemetryTask(MeterConfig& config, TelemetryPublisher& publisher)
        : Task("Telemetry")
        , config(config)
        , publisher(publisher) {
    }

protected:
    const Schedule loop(const Timing& timing) override {
        publisher.publish();
        return sleepFor(config.updateFrequency.get());
    }

private:
    MeterConfig& config;
    TelemetryPublisher& publisher;
};

class FlowMeterApp
    : public Application {
public:
    FlowMeterApp()
        : Application("Flow alert", VERSION, deviceConfig, config, wifiProvider) {
        addTask(flowMeter);
        addTask(telemetryTask);
        telemetryPublisher.registerProvider(flowMeter);
    }

protected:
    void beginApp() override {
        flowMeter.begin(FLOW_PIN, LED_PIN);
    }

private:
    FlowMeterDeviceConfig deviceConfig;
    WiFiManagerProvider wifiProvider;
    WiFiClient client;

    MeterConfig config;
    MeterHandler flowMeter { config };
    TelemetryPublisher telemetryPublisher { mqtt() };
    TelemetryTask telemetryTask { config, telemetryPublisher };
};

FlowMeterApp app;

void setup() {
    app.begin();
}

void loop() {
    app.loop();
}
