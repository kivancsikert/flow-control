#pragma once

#include <cmath>

#include "../ValveHandler.hpp"

using namespace std::chrono;
using namespace farmhub::client;

enum class ValveControlStrategyType {
    NormallyOpen,
    NormallyClosed,
    Latching
};

class ValveControlStrategy {
public:
    virtual void open() = 0;
    virtual void close() = 0;
    virtual String describe() = 0;
};

class Drv8801ValveController
    : public ValveController {

private:
    const uint8_t PWM_PHASE = 0;                                  // PWM channel for phase
    const uint8_t PWM_RESOLUTION = 8;                             // 8 bit
    const int PMW_MAX_VALUE = std::pow(2, PWM_RESOLUTION) - 1;    // 2 ^ PWM_RESOLUTION - 1
    const uint32_t PWM_FREQ = 25000;                              // 25kHz

public:
    class Config
        : public NamedConfigurationSection {
    public:
        Config(ConfigurationSection* parent)
            : NamedConfigurationSection(parent, "valve") {
        }

        Property<ValveControlStrategyType> strategy { this, "strategy", ValveControlStrategyType::NormallyClosed };
        Property<milliseconds> switchDuration { this, "switchDuration", milliseconds { 500 } };
        Property<double> holdDuty { this, "holdDuty", 0.5 };
    };

    class HoldingValveControlStrategy
        : public ValveControlStrategy {

    public:
        HoldingValveControlStrategy(Drv8801ValveController& controller, milliseconds switchDuration, double holdDuty)
            : controller(controller)
            , switchDuration(switchDuration)
            , holdDuty(holdDuty) {
        }

    protected:
        void driveAndHold(bool phase) {
            controller.drive(phase, 1.0);
            delay(switchDuration.count());
            controller.drive(phase, holdDuty);
        }

        Drv8801ValveController& controller;
        const milliseconds switchDuration;
        const double holdDuty;
    };

    class NormallyClosedValveControlStrategy
        : public HoldingValveControlStrategy {
    public:
        NormallyClosedValveControlStrategy(Drv8801ValveController& controller, milliseconds switchDuration, double holdDuty)
            : HoldingValveControlStrategy(controller, switchDuration, holdDuty) {
        }

        void open() override {
            driveAndHold(HIGH);
        }
        void close() override {
            controller.stop();
        }
        String describe() override {
            return "normally closed with switch duration " + String((int) switchDuration.count()) + "ms and hold duty " + String(holdDuty * 100) + "%";
        }
    };

    class NormallyOpenValveControlStrategy
        : public HoldingValveControlStrategy {
    public:
        NormallyOpenValveControlStrategy(Drv8801ValveController& controller, milliseconds switchDuration, double holdDuty)
            : HoldingValveControlStrategy(controller, switchDuration, holdDuty) {
        }

        void open() override {
            controller.stop();
        }
        void close() override {
            driveAndHold(LOW);
        }
        String describe() override {
            return "normally open with switch duration " + String((int) switchDuration.count()) + "ms and hold duty " + String(holdDuty * 100) + "%";
        }
    };

    class LatchingValveControlStrategy
        : public ValveControlStrategy {
    public:
        LatchingValveControlStrategy(Drv8801ValveController& controller, milliseconds switchDuration)
            : controller(controller)
            , switchDuration(switchDuration) {
        }

        void open() override {
            controller.drive(HIGH, 1.0);
            delay(switchDuration.count());
            controller.stop();
        }
        void close() override {
            controller.drive(LOW, 1.0);
            delay(switchDuration.count());
            controller.stop();
        }
        String describe() override {
            return "latching with switch duration " + String((int) switchDuration.count()) + "ms";
        }

    private:
        Drv8801ValveController& controller;
        const milliseconds switchDuration;
    };

    Drv8801ValveController(const Config& config)
        : config(config) {
    }

    void begin(
        gpio_num_t enablePin,
        gpio_num_t phasePin,
        gpio_num_t faultPin,
        gpio_num_t sleepPin,
        gpio_num_t mode1Pin,
        gpio_num_t mode2Pin,
        gpio_num_t currentPin) {
        strategy = createStrategy(config);

        Serial.printf("Initializing DRV8801 valve handler on pins enable = %d, phase = %d, fault = %d, sleep = %d, mode1 = %d, mode2 = %d, current = %d, valve is %s\n",
            enablePin, phasePin, faultPin, sleepPin, mode1Pin, mode2Pin, currentPin, strategy->describe().c_str());

        this->enablePin = enablePin;
        this->phasePin = phasePin;
        this->faultPin = faultPin;
        this->sleepPin = sleepPin;
        this->mode1Pin = mode1Pin;
        this->mode2Pin = mode2Pin;
        this->currentPin = currentPin;

        pinMode(enablePin, OUTPUT);
        pinMode(sleepPin, OUTPUT);
        pinMode(mode1Pin, OUTPUT);
        pinMode(mode2Pin, OUTPUT);
        pinMode(phasePin, OUTPUT);
        ledcAttachPin(phasePin, PWM_PHASE);
        ledcSetup(PWM_PHASE, PWM_FREQ, PWM_RESOLUTION);

        pinMode(faultPin, INPUT);
        pinMode(currentPin, INPUT);

        digitalWrite(mode1Pin, HIGH);
        digitalWrite(mode2Pin, HIGH);
    }

    void open() override {
        strategy->open();
    }

    void close() override {
        strategy->close();
    }

    void reset() override {
        stop();
    }

    void stop() {
        digitalWrite(sleepPin, LOW);
        digitalWrite(enablePin, LOW);
    }

    void drive(bool phase, double duty = 1) {
        digitalWrite(sleepPin, HIGH);
        digitalWrite(enablePin, HIGH);

        int dutyValue = PMW_MAX_VALUE / 2 + (phase ? 1 : -1) * (int) (PMW_MAX_VALUE / 2 * duty);
        Serial.printf("Driving valve %s at %f%%\n",
            phase ? "forward" : "reverse",
            duty * 100);
        ledcWrite(PWM_PHASE, dutyValue);
    }

private:
    ValveControlStrategy* createStrategy(const Config& config) {
        switch (config.strategy.get()) {
            case ValveControlStrategyType::NormallyClosed:
                return new NormallyClosedValveControlStrategy(*this, config.switchDuration.get(), config.holdDuty.get());
            case ValveControlStrategyType::NormallyOpen:
                return new NormallyOpenValveControlStrategy(*this, config.switchDuration.get(), config.holdDuty.get());
            case ValveControlStrategyType::Latching:
                return new LatchingValveControlStrategy(*this, config.switchDuration.get());
            default:
                fatalError("Unknown strategy");
                throw "Unknown strategy";
        }
    }

    const Config& config;
    ValveControlStrategy* strategy;

    gpio_num_t enablePin;
    gpio_num_t phasePin;
    gpio_num_t faultPin;
    gpio_num_t sleepPin;
    gpio_num_t mode1Pin;
    gpio_num_t mode2Pin;
    gpio_num_t currentPin;
};

bool convertToJson(const ValveControlStrategyType& src, JsonVariant dst) {
    switch (src) {
        case ValveControlStrategyType::NormallyOpen:
            return dst.set("NO");
        case ValveControlStrategyType::NormallyClosed:
            return dst.set("NC");
        case ValveControlStrategyType::Latching:
            return dst.set("latching");
        default:
            Serial.println("Unknown strategy: " + String(static_cast<int>(src)));
            return dst.set("NC");
    }
}
void convertFromJson(JsonVariantConst src, ValveControlStrategyType& dst) {
    String strategy = src.as<String>();
    if (strategy == "NO") {
        dst = ValveControlStrategyType::NormallyOpen;
    } else if (strategy == "NC") {
        dst = ValveControlStrategyType::NormallyClosed;
    } else if (strategy == "latching") {
        dst = ValveControlStrategyType::Latching;
    } else {
        Serial.println("Unknown strategy: " + strategy);
        dst = ValveControlStrategyType::NormallyClosed;
    }
}
