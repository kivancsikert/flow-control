#pragma once

#include <FlowMeter.h>
#include <chrono>

#include <Task.hpp>
#include <Telemetry.hpp>

using namespace std::chrono;
using namespace farmhub::client;

// TODO Make these configurable
const auto NO_FLOW_TIMEOUT = seconds { 10 };
const auto SLEEP_PERIOD = minutes { 1 };

FlowMeter* __meterInstance;

IRAM_ATTR void __meterHandlerCountCallback() {
    __meterInstance->count();
}

class MeterHandler
    : public Task,
      public TelemetryProvider {
public:
    MeterHandler()
        : Task("Flow meter") {
    }

    void begin(gpio_num_t flowPin, gpio_num_t ledPin) {
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, HIGH);
        this->flowPin = flowPin;
        this->ledPin = ledPin;

        meter = new FlowMeter(digitalPinToInterrupt(flowPin), UncalibratedSensor, __meterHandlerCountCallback, RISING);
        __meterInstance = meter;

        auto now = boot_clock::now();
        lastMeasurement = now;
        lastSeenFlow = now;
    }

protected:
    const Schedule loop(time_point<boot_clock> scheduledTime) override {
        auto elapsed = duration_cast<milliseconds>(scheduledTime - lastMeasurement);
        lastMeasurement = scheduledTime;
        // TODO Contribute to FlowMeter (otherwise it will result in totals be NaN)
        if (elapsed.count() == 0) {
            return sleepFor(milliseconds { 500 });
        }
        meter->tick(elapsed.count());

        double flowRate = meter->getCurrentFlowrate();
        if (flowRate == 0.0) {
            auto timeSinceLastFlow = scheduledTime - lastSeenFlow;
            if (timeSinceLastFlow > NO_FLOW_TIMEOUT) {
                Serial.printf("No flow for %ld seconds, going to sleep for %ld seconds or until woken up by flow\n",
                    (long) duration_cast<seconds>(timeSinceLastFlow).count(),
                    (long) duration_cast<seconds>(SLEEP_PERIOD).count());
                Serial.flush();

                // Disable interrupt to avoid crashing on wakeup
                // TODO Contribute a stop() method to FlowMeter
                detachInterrupt(digitalPinToInterrupt(flowPin));

                // Turn off LED
                digitalWrite(ledPin, LOW);

                // Go to deep sleep until timeout or woken up by GPIO interrupt
                esp_sleep_enable_timer_wakeup(duration_cast<microseconds>(SLEEP_PERIOD).count());
                esp_sleep_enable_ext0_wakeup(flowPin, digitalRead(flowPin) == LOW);
                esp_deep_sleep_start();
            }
        } else {
            lastSeenFlow = scheduledTime;
        }
        return sleepFor(milliseconds { 500 });
    }

    void populateTelemetry(JsonObject& json) override {
        json["model"] = "flow-alert@mk1";
        json["description"] = "Flow alerter";
        json["flowRate"] = meter->getCurrentFlowrate();
        json["volume"] = meter->getCurrentVolume();
        json["totalFlowRate"] = meter->getTotalFlowrate();
        json["totalVolume"] = meter->getTotalVolume();
    }

private:
    gpio_num_t flowPin;
    gpio_num_t ledPin;
    FlowMeter* meter;
    time_point<boot_clock> lastMeasurement;
    time_point<boot_clock> lastSeenFlow;
};
