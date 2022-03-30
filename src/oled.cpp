#include "oled.h"
#include "board.h"

void Oled::loop() {
    ulong t = millis();
    if ((lastPower < t - 5000) && (lastCadence < t - 5000)) {
        if (Atoll::systemTimeLastSet())
            showTime();
        else
            showSatellites();
    }
    if (lastHeartrate < t - 3000)
        if (Atoll::systemTimeLastSet())
            showDate();
}

void Oled::showSatellites() {
    printfField(1, true, 1, 0, "-%02d", board.gps.satellites());
}