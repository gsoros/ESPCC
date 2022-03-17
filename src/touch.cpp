#include "touch.h"
#include "board.h"

void Touch::fireEvent(uint8_t index, TouchEvent event) {
    Atoll::Touch::fireEvent(index, event);
    board.oled.onTouchEvent(&pads[index], event);
    if (event == TouchEvent::longTouch)
        board.bleClient.startScan(5);
}