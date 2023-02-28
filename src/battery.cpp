#include "board.h"
#include "battery.h"

bool Battery::report() {
    bool sent = Atoll::Battery::report();
    static Battery::ChargingState prevState = csUnknown;
    if (sent || prevState != chargingState)
        board.display.onBattery((int8_t)level, chargingState);
    prevState = chargingState;
    return sent;
}