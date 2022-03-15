#include "oled.h"
#include "board.h"
#include "touch.h"

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

    assert(pad->index < sizeof(feedback) / sizeof(feedback[0]));
    Area *a = &feedback[pad->index];

    if (TouchEvent::end == event) {
        log_i("pad %d touch", pad->index);
        fill(a, 1);
        fill(a, 0);
        return;
    }
    if (TouchEvent::doubleTouch == event) {
        log_i("pad %d double touch", pad->index);
        Area b;
        memcpy(&b, a, sizeof(b));
        b.h /= 3;
        fill(&b, 1, false);
        b.y += b.h;
        fill(&b, 0, false);
        b.y += b.h;
        fill(&b, 1);
        delay(Touch::touchTime * 3);
        fill(a, 0);
        return;
    }
    if (TouchEvent::longTouch == event) {
        log_i("pad %d long touch", pad->index);
        fill(a, 1);
        delay(Touch::touchTime * 3);
        fill(a, 0);
        return;
    }
    if (TouchEvent::touching == event) {
        // log_i("touching");
        if (pad->start + Touch::longTouchTime < millis()) {  // animation completed, still touching
            fill(a, 1);
            delay(Touch::touchTime * 3);
            fill(a, 0);
            return;
        }
        // log_i("animating");
        const ulong t = millis();
        Area b;
        memcpy(&b, a, sizeof(b));
        b.h = map(t - pad->start, 0, Touch::longTouchTime, 0, b.h);  // scale down area height
        if (a->h < b.h) return;                                      // overflow
        b.y += (a->h - b.h) / 2;                                     // move area to vertical middle
        fill(a, 0, false);
        fill(&b, 1);
    }
}
