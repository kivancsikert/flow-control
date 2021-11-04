// include the library code:
#include <Arduino.h>
#include <ArduinoJson.hpp>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <chrono>

#include <HttpUpdateHandler.hpp>
#include <MqttHandler.hpp>
#include <OtaHandler.hpp>
#include <Telemetry.hpp>

#include "MeterHandler.hpp"
#include "TelemetryHandler.hpp"
#include "version.h"

using namespace std::chrono;
using namespace farmhub::client;

const gpio_num_t FLOW_PIN = GPIO_NUM_33;
const gpio_num_t LED_PIN = GPIO_NUM_19;

WiFiClient client;

MqttHandler mqtt;
MeterHandler flowMeter;
TelemetryPublisher telemetryPublisher(mqtt, "events");
TelemetryHandler telemetry(telemetryPublisher);
OtaHandler otaHandler;
HttpUpdateHandler httpUpdateHandler(mqtt, VERSION);

void fatalError(String message) {
    Serial.println(message);
    Serial.flush();
    delay(10000);
    ESP.restart();
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    if (!SPIFFS.begin()) {
        fatalError("Could not initialize file system");
    }

    flowMeter.begin(FLOW_PIN, LED_PIN);

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
        mqttConfigJson.as<JsonObject>(),
        [](const JsonObject& json) {
            Serial.println("Cannot update config yet");
        });

    httpUpdateHandler.begin();
    otaHandler.begin(hostname.c_str());

    telemetryPublisher.registerProvider(flowMeter);
    telemetry.begin();
}

void loop() {
    flowMeter.loop();
    telemetry.loop();
    mqtt.loop();

    delay(100);
}
