//#include "board.h"
#include "oled.h"

Oled::~Oled() {
    delete device;
}
