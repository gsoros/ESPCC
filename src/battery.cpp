#include "board.h"
#include "battery.h"

bool Battery::report() {
    bool res = Atoll::Battery::report();
    if (res)
        board.display.onBattery((int8_t)level);
    return res;
}