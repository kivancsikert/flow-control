#ifdef MK3
#include "mk3/FlowControlApp.hpp"
#elif MK4
#include "mk4/FlowControlApp.hpp"
#endif

FlowControlApp app;

void setup() {
    app.begin();
}

void loop() {
    app.loop();
}
