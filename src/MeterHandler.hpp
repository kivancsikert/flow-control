#pragma once

#include <FlowMeter.h>
#include <chrono>

#include <Task.hpp>
#include <Telemetry.hpp>

using namespace std::chrono;
using namespace farmhub::client;

FlowMeter* __meterInstance;

IRAM_ATTR void __meterHandlerCountCallback() {
    __meterInstance->count();
}

class MeterConfig
    : public FileConfiguration {
public:
    MeterConfig()
        : FileConfiguration("application", "/config.json") {
    }

    Property<seconds> publishInterval { serializer, "publishInterval", minutes { 1 } };
    Property<seconds> noFlowTimeout { serializer, "noFlowTimeout", minutes { 10 } };
    Property<seconds> sleepPeriod { serializer, "sleepPeriod", hours { 1 } };
};

class MeterHandler
    : public Task,
      public TelemetryProvider {
public:
    MeterHandler(MeterConfig& config)
        : Task("Flow meter")
        , config(config) {
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
    const Schedule loop(const Timing& timing) override {
        auto now = boot_clock::now();
        milliseconds elapsed = duration_cast<milliseconds>(now - lastMeasurement);
        lastMeasurement = now;
        // TODO Contribute to FlowMeter (otherwise it will result in totals be NaN)
        if (elapsed.count() == 0) {
            return sleepFor(config.publishInterval.get());
        }
        meter->tick(elapsed.count());

        double flowRate = meter->getCurrentFlowrate();
        if (flowRate == 0.0 && config.noFlowTimeout.get() > seconds::zero() && config.sleepPeriod.get() > seconds::zero()) {
            auto timeSinceLastFlow = now - lastSeenFlow;
            if (timeSinceLastFlow > config.noFlowTimeout.get()) {
                Serial.printf("No flow for %ld seconds, going to sleep for %ld seconds or until woken up by flow\n",
                    (long) duration_cast<seconds>(timeSinceLastFlow).count(),
                    (long) duration_cast<seconds>(config.sleepPeriod.get()).count());
                Serial.flush();

                // Disable interrupt to avoid crashing on wakeup
                // TODO Contribute a stop() method to FlowMeter
                detachInterrupt(digitalPinToInterrupt(flowPin));

                // Turn off LED
                digitalWrite(ledPin, LOW);

                // Go to deep sleep until timeout or woken up by GPIO interrupt
                esp_sleep_enable_timer_wakeup(duration_cast<microseconds>(config.sleepPeriod.get()).count());
                esp_sleep_enable_ext0_wakeup(flowPin, digitalRead(flowPin) == LOW);
                esp_deep_sleep_start();
            }
        } else {
            lastSeenFlow = now;
        }
        return sleepFor(config.publishInterval.get());
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
    MeterConfig& config;
    gpio_num_t flowPin;
    gpio_num_t ledPin;
    FlowMeter* meter;
    time_point<boot_clock> lastMeasurement;
    time_point<boot_clock> lastSeenFlow;
};
