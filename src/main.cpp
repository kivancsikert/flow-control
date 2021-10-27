// include the library code:
#include <Arduino.h>
#include <FlowMeter.h>
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

    meter = new FlowMeter(digitalPinToInterrupt(FLOW_PIN), UncalibratedSensor, meterCount, RISING);
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
