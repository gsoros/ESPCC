#include "battery.h"
#include "board.h"

bool Battery::report() {
    bool res = Atoll::Battery::report();
    if (res)
        board.oled.onBattery((int8_t)level);
    return res;
}