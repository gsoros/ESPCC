#include "board.h"
#include "gps.h"

void GPS::loop() {
    Atoll::GPS::loop();

    if (device.speed.isValid() && device.speed.isUpdated())
        board.display.onSpeed(device.speed.kmph());
}