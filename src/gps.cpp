#include "board.h"
#include "gps.h"

void GPS::loop() {
    Atoll::GPS::loop();

    ulong t = millis();

    static ulong lastSpeedCheck = 0;
    if (lastSpeedCheck + 2000 < t) {
        if (device.speed.isValid()) {
            if (device.speed.isUpdated())
                board.display.onSpeed(device.speed.kmph());
        } else
            board.display.onSpeed(-1.0);
        lastSpeedCheck = t;
    }

    static ulong lastSatCheck = 0;
    if (lastSatCheck + 5000 < t) {
        if (device.satellites.isValid()) {
            if (device.satellites.isUpdated())
                board.display.onSatellites(device.satellites.value());
        } else
            board.display.onSatellites(UINT32_MAX);
        lastSatCheck = t;
    }
}