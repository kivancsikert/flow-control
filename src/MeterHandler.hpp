#pragma once

#include <FlowMeter.h>
#include <chrono>

#include <Task.hpp>
#include <Telemetry.hpp>

using namespace std::chrono;
using namespace farmhub::client;

FlowMeter* __meterInstance = nullptr;

IRAM_ATTR void __meterHandlerCountCallback() {
    if (__meterInstance != nullptr) {
        __meterInstance->count();
    }
}

class MeterHandler
    : public BaseTask,
      public BaseSleepListener,
      public TelemetryProvider {
public:
    class Config
        : public NamedConfigurationSection {
    public:
        Config(ConfigurationSection* parent)
            : NamedConfigurationSection(parent, "meter") {
        }

        Property<seconds> measurementFrequency { this, "measurementFrequency", seconds { 1 } };
        Property<seconds> noFlowTimeout { this, "noFlowTimeout", minutes { 10 } };
    };

    MeterHandler(TaskContainer& tasks, SleepHandler& sleep, const Config& config, std::function<void()> onSleep)
        : BaseTask(tasks, "Flow meter")
        , BaseSleepListener(sleep)
        , config(config)
        , onSleep(onSleep) {
    }

    void begin(gpio_num_t flowPin) {
        this->flowPin = flowPin;

        noInterrupts();
        meter = new FlowMeter(digitalPinToInterrupt(flowPin), UncalibratedSensor, __meterHandlerCountCallback, RISING);
        __meterInstance = meter;
        interrupts();

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
            return sleepFor(config.measurementFrequency.get());
        }
        meter->tick(elapsed.count());

        double flowRate = meter->getCurrentFlowrate();
        if (flowRate == 0.0 && config.noFlowTimeout.get() > seconds::zero()) {
            auto timeSinceLastFlow = now - lastSeenFlow;
            if (timeSinceLastFlow > config.noFlowTimeout.get()) {
                Serial.printf("No flow for %ld seconds\n",
                    (long) duration_cast<seconds>(timeSinceLastFlow).count());
                onSleep();
            }
        } else {
            lastSeenFlow = now;
        }
        return sleepFor(config.measurementFrequency.get());
    }

    void onDeepSleep(SleepEvent& event) override {
        Serial.println("Wake up on flow");
        esp_sleep_enable_ext0_wakeup(flowPin, digitalRead(flowPin) == LOW);
    }

    void populateTelemetry(JsonObject& json) override {
        json["volume"] = meter->getCurrentVolume();
    }

private:
    const Config& config;
    std::function<void()> onSleep;
    gpio_num_t flowPin;
    FlowMeter* meter;
    time_point<boot_clock> lastMeasurement;
    time_point<boot_clock> lastSeenFlow;
};
