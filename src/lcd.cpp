#include "board.h"
#include "lcd.h"

Lcd::~Lcd() {
    delete device;
}

void Lcd::fill(const Area *a, uint16_t color, bool send) {
    if (board.otaMode) return;
    fillOta(a, color, send);
}

void Lcd::clock(bool send, bool clear, int8_t skipFieldIndex) {
    // log_i("send: %d clear: %d skip: %d", send, clear, skipFieldIndex);
    if (board.otaMode) return;
    static const Area *a = &clockArea;
    if (-2 == lastMinute) return;  // avoid recursion
    tm t = Atoll::localTm();
    if (t.tm_min == lastMinute && !clear) return;
    if (send && !aquireMutex()) return;
    setClip(a->x, a->y, a->w, a->h);
    fill(a, bg, false);
    if (clear) {
        setMaxClip();
        lastMinute = -2;  // avoid recursion
        for (uint8_t i = 0; i < numFields; i++)
            if ((int8_t)i != skipFieldIndex)
                displayFieldContent(i,
                                    field[i].content[currentPage],
                                    false);
        lastMinute = t.tm_min;
        if (!send) return;
        sendBuffer();
        releaseMutex();
        return;
    }
    setCursor(a->x, a->y + a->h / 2);
    setFont(timeFont);
    Arduino_Canvas::printf("%d:%02d", t.tm_hour, t.tm_min);
    setCursor(a->x, a->y + a->h);
    setFont(dateFont);
    Arduino_Canvas::printf("%d.%02d", t.tm_mon + 1, t.tm_mday);
    setMaxClip();
    lastMinute = t.tm_min;
    if (!send) return;
    sendBuffer();
    releaseMutex();
}
