#include "board.h"
#include "touch.h"

void Touch::fireEvent(uint8_t index, Event event) {
    Atoll::Touch::fireEvent(index, event);
    if (!board.display.onTouchEvent(&pads[index], event)) {
        return;
    }
    switch (event) {
        case Event::longTouch: {
            if (board.recorder.start()) {
                // disable wifi but don't save
                board.wifi.setEnabled(false, false);
            }
        } break;
        case Event::doubleTouch: {
            if (board.recorder.end()) {
                if (board.wifi.startOnRecordingEnd) {
                    log_i("starting wifi");
                    board.wifi.autoStartWebserver = true;
                    board.wifi.autoStartWifiSerial = false;
                    // enable wifi but don't save
                    board.wifi.setEnabled(true, false);
                }
            }
        } break;
        default:
            break;
    }
}