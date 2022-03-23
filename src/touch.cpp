#include "touch.h"
#include "board.h"

void Touch::fireEvent(uint8_t index, Event event) {
    Atoll::Touch::fireEvent(index, event);
    board.oled.onTouchEvent(&pads[index], event);
    if (event == Event::longTouch)
        board.bleClient.startScan(5);
    else if (event == Event::doubleTouch) {
        for (uint8_t i = 0; i < board.bleClient.peersMax; i++) {
            if (board.bleClient.peers[i] != nullptr) {
                board.bleClient.peers[i]->disconnect();
                board.bleClient.removePeer(board.bleClient.peers[i]);
                board.bleClient.saveSettings();
                break;
            }
        }
    }
}