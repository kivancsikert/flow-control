// include the library code:
#include <Arduino.h>
#include <FlowMeter.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <chrono>

using namespace std::chrono;

const gpio_num_t FLOW_PIN = GPIO_NUM_33;

const auto MEASUREMENT_PERIOD = seconds { 1 };
const auto NO_FLOW_TIMEOUT = seconds { 5 };
const auto SLEEP_PERIOD = minutes { 1 };

// get a new FlowMeter instance for an uncalibrated flow sensor
FlowMeter* meter;

auto lastSeenFlow = milliseconds::zero();

IRAM_ATTR void meterCount() {
    meter->count();
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("Hello");

    WiFi.mode(WIFI_STA);    // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.

    // put your setup code here, to run once:
    Serial.begin(115200);

    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    //wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("AutoConnectAP", "password");    // password protected ap

    if (!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
    }

    meter = new FlowMeter(digitalPinToInterrupt(FLOW_PIN), UncalibratedSensor, meterCount, RISING);

    lastSeenFlow = milliseconds { millis() };
}

void loop() {
    // process the (possibly) counted ticks
    auto measurementMillis = duration_cast<milliseconds>(MEASUREMENT_PERIOD).count();
    delay(measurementMillis);
    meter->tick(measurementMillis);

    double flowRate = meter->getCurrentFlowrate();
    auto currentTime = milliseconds { millis() };
    if (flowRate == 0.0) {
        if ((currentTime - lastSeenFlow) > NO_FLOW_TIMEOUT) {
            Serial.println("No flow for 5 seconds, going to sleep");
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
}
