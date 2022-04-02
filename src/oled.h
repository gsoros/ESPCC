#ifndef __oled_h
#define __oled_h

#include "touch.h"  // avoid "NUM_PADS redefined"
#include "atoll_oled.h"

class Oled : public Atoll::Oled {
   public:
    Oled(
        U8G2 *device,
        uint8_t width = 64,
        uint8_t height = 128,
        uint8_t feedbackWidth = 3,
        uint8_t fieldHeight = 32)
        : Atoll::Oled(
              device,
              width,
              height,
              feedbackWidth,
              fieldHeight) {}

    void setup();
    void loop();
    void showSatellites();
    void animateRecording(bool clear = false);
};

#endif