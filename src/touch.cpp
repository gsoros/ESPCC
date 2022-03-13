#include "touch.h"
#include "board.h"

void Touch::onEvent(uint8_t index, TouchEvent event) {
    Atoll::Touch::onEvent(index, event);
    board.oled.onTouchEvent(&pads[index], event);
}