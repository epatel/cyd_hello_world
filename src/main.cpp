#include "envreader.h"
#include "display.h"

EnvReader envReader;
Display display(envReader);

void setup() {
    Serial.begin(115200);

    envReader.init();
    display.init();
}

void loop() {
    envReader.tick();
    display.tick();
}
