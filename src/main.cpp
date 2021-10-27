// include the library code:
#include <Arduino.h>
#include <FlowMeter.h>
#include <Wire.h>

const gpio_num_t FLOW_PIN = GPIO_NUM_33;

// get a new FlowMeter instance for an uncalibrated flow sensor
FlowMeter* meter;

long long lastSeenFlow = 0;

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
    delay(1000);
    meter->tick(1000);

    double flowRate = meter->getCurrentFlowrate();
    auto currentTime = millis();
    if (flowRate > 0.0) {
        Serial.print("Flow rate: ");
        Serial.println(flowRate);
        lastSeenFlow = currentTime;
    } else {
        if (currentTime - lastSeenFlow > 5000) {
            Serial.println("No flow for 5 seconds, going to sleep");
            // Go to deep sleep until woken up by GPIO interrupt
            esp_sleep_enable_ext0_wakeup(FLOW_PIN, digitalRead(FLOW_PIN) == LOW);
            esp_deep_sleep_start();
        }
    }

    // output some measurement result
    Serial.print("Currently ");
    Serial.print(flowRate);
    Serial.print(" l/min, ");
    Serial.print(meter->getTotalVolume());
    Serial.println(" l total.");
}
