#include "board.h"
#include "touch.h"

void Touch::fireEvent(uint8_t index, Event event) {
    Atoll::Touch::fireEvent(index, event);
    board.oled.onTouchEvent(&pads[index], event);
    switch (event) {
        case Event::longTouch: {
            if (board.recorder.start()) {
                // disable wifi but don't save
                board.wifi.setEnabled(false, false);
            }
        } break;
        case Event::doubleTouch: {
            if (board.recorder.end()) {
                // enable wifi but don't save
                board.wifi.setEnabled(true, false);
            }
        } break;
        default:
            break;
    }
}