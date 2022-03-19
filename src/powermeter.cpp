#include "powermeter.h"
#include "board.h"

void PowerChar::onNotify(
    BLERemoteCharacteristic* c,
    uint8_t* data,
    size_t length,
    bool isNotify) {
    lastValue = decode(data, length);
    // log_i("PowerChar::onNotify %d %d", lastValue, lastCadence);
    board.oled.displayPower(lastValue);
    board.oled.displayCadence(lastCadence);
}