#pragma once

#include <chrono>
#include <driver/pcnt.h>

#include <Task.hpp>
#include <Telemetry.hpp>

using namespace std::chrono;
using namespace farmhub::client;

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

    MeterHandler(
        TaskContainer& tasks, SleepHandler& sleep, const Config& config, std::function<void()> onSleep)
        : BaseTask(tasks, "Flow meter")
        , BaseSleepListener(sleep)
        , config(config)
        , onSleep(onSleep) {
    }

    void begin(gpio_num_t flowPin, double qFactor) {
        this->flowPin = flowPin;
        this->qFactor = qFactor;
        Serial.printf("Initializing flow meter on pin %d with Q = %f\n", flowPin, qFactor);

        pinMode(flowPin, INPUT);

        pcnt_config_t pcntFreqConfig = {};
        pcntFreqConfig.pulse_gpio_num = flowPin;
        pcntFreqConfig.ctrl_gpio_num = PCNT_PIN_NOT_USED;
        pcntFreqConfig.lctrl_mode = PCNT_MODE_KEEP;
        pcntFreqConfig.hctrl_mode = PCNT_MODE_KEEP;
        pcntFreqConfig.pos_mode = PCNT_COUNT_INC;
        pcntFreqConfig.neg_mode = PCNT_COUNT_DIS;
        pcntFreqConfig.unit = PCNT_UNIT_0;
        pcntFreqConfig.channel = PCNT_CHANNEL_0;

        pcnt_unit_config(&pcntFreqConfig);
        pcnt_intr_disable(PCNT_UNIT_0);
        pcnt_set_filter_value(PCNT_UNIT_0, 1023);
        pcnt_filter_enable(PCNT_UNIT_0);
        pcnt_counter_clear(PCNT_UNIT_0);

        auto now = boot_clock::now();
        lastMeasurement = now;
        lastSeenFlow = now;
    }

protected:
    const Schedule loop(const Timing& timing) override {
        auto now = boot_clock::now();
        milliseconds elapsed = duration_cast<milliseconds>(now - lastMeasurement);
        if (elapsed.count() == 0) {
            return sleepFor(config.measurementFrequency.get());
        }
        lastMeasurement = now;

        int16_t pulses;
        pcnt_get_counter_value(PCNT_UNIT_0, &pulses);
        pcnt_counter_clear(PCNT_UNIT_0);

        if (pulses == 0) {
            if (config.noFlowTimeout.get() > seconds::zero()) {
                auto timeSinceLastFlow = now - lastSeenFlow;
                if (timeSinceLastFlow > config.noFlowTimeout.get()) {
                    Serial.printf("No flow for %ld seconds\n",
                        (long) duration_cast<seconds>(timeSinceLastFlow).count());
                    onSleep();
                }
            }
        } else {
            double currentVolume = pulses / qFactor / 60.0f;
            Serial.printf("Counted %d pulses, %.2f l/min, %.2f l\n",
                pulses, currentVolume / (elapsed.count() / 1000.0f / 60.0f), currentVolume);
            volume += currentVolume;
            lastSeenFlow = now;
        }
        return sleepFor(config.measurementFrequency.get());
    }

    void onDeepSleep(SleepEvent& event) override {
        Serial.println("Wake up on flow");
        esp_sleep_enable_ext0_wakeup(flowPin, digitalRead(flowPin) == LOW);
    }

    void populateTelemetry(JsonObject& json) override {
        json["volume"] = volume;
        volume = 0.0;
    }

private:
    const Config& config;
    std::function<void()> onSleep;
    gpio_num_t flowPin;
    double qFactor;

    time_point<boot_clock> lastMeasurement;
    time_point<boot_clock> lastSeenFlow;
    double volume = 0.0;
};
