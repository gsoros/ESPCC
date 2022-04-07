#include "gps.h"
#include "board.h"

void GPS::loop() {
    Atoll::GPS::loop();

    if (gps.speed.isValid() && gps.speed.isUpdated())
        board.oled.onSpeed(gps.speed.kmph());
}