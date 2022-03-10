#include <Arduino.h>

#include "board.h"
#include "gps.h"
#include "oled.h"

Board board;

void setup() {
    Serial.begin(115200);
    Serial.printf("\n\n\nESPCC %s %s\n\n\n", __DATE__, __TIME__);
    board.setup();
}

void loop() {
    vTaskDelete(NULL);
}