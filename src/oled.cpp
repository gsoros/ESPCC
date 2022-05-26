#include "board.h"
#include "oled.h"

Oled::~Oled() {
    delete device;
}

void Oled::fill(const Area *a, uint16_t color, bool send) {
    if (board.otaMode) return;
    fillOta(a, color, send);
}

void Oled::clock(bool send, bool clear, int8_t skipFieldIndex) {
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
        if (!send) return;
        sendBuffer();
        releaseMutex();
        return;
    }
    setCursor(a->x + a->w - timeFontHeight - 1, a->y);
    setFont(timeFont);
    device->setFontDirection(1);  // 90˚
    printf("%d:%02d", t.tm_hour, t.tm_min);
    setCursor(a->x, a->y);
    setFont(dateFont);
    printf("%d.%02d", t.tm_mon + 1, t.tm_mday);
    device->setFontDirection(0);
    setMaxClip();
    lastMinute = t.tm_min;
    if (!send) return;
    sendBuffer();
    releaseMutex();
}
