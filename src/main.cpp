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

class FlowMeterConfig : public FileConfiguration {
public:
    FlowMeterConfig()
        : FileConfiguration("application", "/config.json") {
    }
};

class FlowMeterApp
    : public Application {
public:
    FlowMeterApp()
        : Application("Flow meter", VERSION, appConfig, wifiProvider)
        , telemetryPublisher(mqtt, "events")
        , telemetryTask("Publish telemetry", seconds { 5 }, [&]() {
            telemetryPublisher.publish();
        }) {
        addTask(flowMeter);
        addTask(telemetryTask);
        telemetryPublisher.registerProvider(flowMeter);
    }

protected:
    void beginApp() override {
        flowMeter.begin(FLOW_PIN, LED_PIN);
    }

private:
    FlowMeterConfig appConfig;
    WiFiManagerProvider wifiProvider;

    WiFiClient client;
    MeterHandler flowMeter;
    TelemetryPublisher telemetryPublisher;
    IntervalTask telemetryTask;
};

FlowMeterApp app;

void setup() {
    app.begin("flow-alert");
}

void loop() {
    app.loop();
}
