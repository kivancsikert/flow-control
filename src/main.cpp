// include the library code:
#include <Arduino.h>
#include <ArduinoJson.hpp>
#include <ESPmDNS.h>
#include <FlowMeter.h>
#include <MqttHandler.hpp>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <chrono>

#include "version.h"

using namespace std::chrono;
using namespace Mqtt;

const gpio_num_t FLOW_PIN = GPIO_NUM_33;
const gpio_num_t LED_PIN = GPIO_NUM_19;

const auto MEASUREMENT_PERIOD = seconds { 1 };
const auto NO_FLOW_TIMEOUT = seconds { 5 };
const auto SLEEP_PERIOD = minutes { 1 };

// get a new FlowMeter instance for an uncalibrated flow sensor
FlowMeter* meter;

WiFiClient client;

MqttHandler mqtt;

auto lastSeenFlow = milliseconds::zero();
auto lastMeasurement = milliseconds::zero();

IRAM_ATTR void meterCount() {
    meter->count();
}

void fatalError(String message) {
    Serial.println(message);
    Serial.flush();
    delay(10000);
    ESP.restart();
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    if (!SPIFFS.begin()) {
        fatalError("Could not initialize file system");
    }

    // Explicitly set mode, ESP defaults to STA+AP
    WiFi.mode(WIFI_STA);

    WiFiManager wm;

    // Reset settings - wipe stored credentials for testing;
    // these are stored by the ESP library
    //wm.resetSettings();

    // Allow some time for connecting to the WIFI, otherwise
    // open configuration portal
    wm.setConnectTimeout(20);

    // Close the configuration portal after some time and reboot
    // if no WIFI is configured in that time
    wm.setConfigPortalTimeout(300);

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point
    // with an auto-generated SSID and no password,
    // then goes into a blocking loop awaiting
    // configuration and will return success result.
    if (!wm.autoConnect()) {
        fatalError("Failed to connect to WIFI");
    }

    // TODO Make configurable
    String hostname = "flow-alert";

    WiFi.setHostname(hostname.c_str());
    MDNS.begin(hostname.c_str());

    File mqttConfigFile = SPIFFS.open("/mqtt-config.json", FILE_READ);
    DynamicJsonDocument mqttConfigJson(mqttConfigFile.size() * 2);
    DeserializationError error = deserializeJson(mqttConfigJson, mqttConfigFile);
    if (error) {
        Serial.println(mqttConfigFile.readString());
        fatalError("Failed to read MQTT config file at /mqtt-config.json: " + String(error.c_str()));
    }
    mqtt.begin(
        client,
        mqttConfigJson.as<JsonObject>(),
        [](const JsonObject& json) {
            Serial.println("Cannot update config yet");
        },
        [](const JsonObject& json) {
            Serial.println("Unknown command");
        });

    meter = new FlowMeter(digitalPinToInterrupt(FLOW_PIN), UncalibratedSensor, meterCount, RISING);

    lastSeenFlow = milliseconds { millis() };
    lastMeasurement = milliseconds { millis() };
}

void loop() {
    mqtt.loop();

    // process the (possibly) counted ticks
    auto measurementMillis = duration_cast<milliseconds>(MEASUREMENT_PERIOD).count();
    delay(measurementMillis);
    meter->tick(measurementMillis);

    double flowRate = meter->getCurrentFlowrate();
    auto currentTime = milliseconds { millis() };
    if (flowRate == 0.0) {
        auto timeSinceLastFlow = currentTime - lastSeenFlow;
        if (timeSinceLastFlow > NO_FLOW_TIMEOUT) {
            Serial.printf("No flow for %ld seconds, going to sleep for %ld seconds or until woken up by flow\n",
                (long) duration_cast<seconds>(timeSinceLastFlow).count(),
                (long) duration_cast<seconds>(SLEEP_PERIOD).count());
            Serial.flush();
            digitalWrite(LED_PIN, LOW);
            // Go to deep sleep until timeout or woken up by GPIO interrupt
            esp_sleep_enable_timer_wakeup(duration_cast<microseconds>(SLEEP_PERIOD).count());
            esp_sleep_enable_ext0_wakeup(FLOW_PIN, digitalRead(FLOW_PIN) == LOW);
            esp_deep_sleep_start();
        }
    } else {
        lastSeenFlow = currentTime;
    }

    // output some measurement result
    Serial.print("Currently ");
    Serial.print(flowRate);
    Serial.print(" l/min, ");
    Serial.print(meter->getTotalVolume());
    Serial.println(" l total.");

    DynamicJsonDocument doc(1024);
    JsonObject root = doc.to<JsonObject>();
    root["model"] = "flow-alert@mk1";
    root["description"] = "Flow alerter";
    root["firmware"] = VERSION;
    root["flow"] = flowRate;
    mqtt.publish("status", doc);
}
