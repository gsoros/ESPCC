#include <Arduino.h>

#include "board.h"
#include "gps.h"
#include "oled.h"

Board board;

void setup() {
    Serial.begin(115200);
    board.setup();
}

void loop() {
    vTaskDelete(NULL);
}