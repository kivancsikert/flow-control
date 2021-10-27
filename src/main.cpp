// include the library code:
#include <Arduino.h>
#include <FlowMeter.h>
#include <Wire.h>

const int FLOW_PIN = GPIO_NUM_33;

// get a new FlowMeter instance for an uncalibrated flow sensor
FlowMeter* meter;

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
    delay(200);
    meter->tick(200);

    // output some measurement result
    Serial.print("Currently ");
    Serial.print(meter->getCurrentFlowrate());
    Serial.print(" l/min, ");
    Serial.print(meter->getTotalVolume());
    Serial.println(" l total.");
}
