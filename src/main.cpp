#include "board.h"

Board board;

void setup() {
    board.setup();
}

void loop() {
    vTaskDelete(NULL);
}