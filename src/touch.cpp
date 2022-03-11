#include "touch.h"
#include "board.h"

void Touch::onEvent(uint8_t index, uint8_t event) {
    Atoll::Touch::onEvent(index, event);
    board.oled.onTouchEvent(index, event);
}