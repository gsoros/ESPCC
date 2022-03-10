#ifndef __board_h
#define __board_h

#include "gps.h"
#include "oled.h"

class Board {
   public:
    GPS gps;
    Oled oled;

    Board() {}

    void setup() {
        setCpuFrequencyMhz(80);
        gps.setup();
        oled.setup();
        gps.taskStart("GPS Task", 100);
    }
};

extern Board board;

#endif