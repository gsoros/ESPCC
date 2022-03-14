#include "oled.h"
#include "board.h"

void Oled::loop() {
    /*
    static const uint8_t contrastMax = __UINT8_MAX__;
    static int16_t contrast = contrastMax;
    static int16_t newContrast = contrast;
    if (board.touch.anyPadIsTouched()) {
        static bool increase = true;
        newContrast += increase ? 50 : -50;
        if (newContrast < 0) {
            newContrast = 0;
            increase = true;
        } else if (contrastMax < newContrast) {
            newContrast = contrastMax;
            increase = false;
        }
    } else
        newContrast = contrastMax;
    if (contrast != newContrast) {
        contrast = newContrast;
        // bool success =
        setContrast((uint8_t)contrast);
        // log_i("Contrast: %d %s\n", contrast, success ? "succ" : "fail");
    }
    */
}

void Oled::onTouchEvent(TouchPad *pad, uint8_t event) {
    // printfField(1, true, 0, 1, "%d%02lu",
    //             pad->index, (millis() - pad->start) / 100);

    if (sizeof(feedback) / sizeof(feedback[0]) <= pad->index) return;
    // if (TouchEvent::start == event) {
    //   fill(&feedback[pad->index], 1, true);
    //   return;
    // }
    if (TouchEvent::end == event) {
        // log_i("end");
        fill(&feedback[pad->index], 1);
        fill(&feedback[pad->index], 0);
        return;
    }
    if (TouchEvent::doubleTouch == event) {
        log_i("pad %d double touch", pad->index);
        Area a = feedback[pad->index];  // copy
        a.h /= 3;
        fill(&a, 1, false);
        a.y += a.h;
        fill(&a, 0, false);
        a.y += a.h;
        fill(&a, 1);
        fill(&feedback[pad->index], 0);
        return;
    }
    if (TouchEvent::longTouch == event) {
        log_i("pad %d long touch", pad->index);
        return;
    }
    if (TouchEvent::touching == event) {
        // log_i("touching");
        if (pad->start < pad->longTouch) {  // animation completed, still touching
            // fill(&feedback[pad->index], 1, true);
            return;
        }
        // log_i("animating");
        const ulong t = millis();
        Area a = feedback[pad->index];  // copy
        a.h = map(t - pad->start, 0, Touch::longTouchTime, 0, a.h);
        if (feedback[pad->index].h < a.h) return;  // overflow
        a.y += (feedback[pad->index].h - a.h) / 2;
        fill(&feedback[pad->index], 0, false);
        fill(&a, 1);
    }
}
