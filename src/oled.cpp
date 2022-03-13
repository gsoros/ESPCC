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
    printfField(1, true, "%d%02lu", pad->index, (millis() - pad->touchStart) / 100);

    if (sizeof(feedback) / sizeof(feedback[0]) <= pad->index) return;
    if (TouchEvent::start == event)
        fill(&feedback[pad->index], 1, true);
    else if (TouchEvent::end == event)
        fill(&feedback[pad->index], 0, true);
}
