#include "board.h"
#include "lcd.h"

Lcd::Lcd(Arduino_GFX *device,
         uint8_t width,
         uint8_t height,
         uint8_t feedbackWidth,
         uint8_t fieldHeight,
         SemaphoreHandle_t *mutex)
    : Display(width, height, feedbackWidth, fieldHeight, mutex),
      Arduino_Canvas(width, height, device, 0, 0) {
    /*
            // ┌─┬──────────────┬┬──────────────┬─┐
            // │f│ field0       ││ field1       │f│
            // │e│              ││              │e│
            // │e│              ││              │e│
            // │d│              ││              │d│
            // │b│          c l o c k           │b│
            // │0│              ││              │1│
            // ├─┼──────────────┼┼──────────────┼─┤
            // │f├──────────────┼┼──────────────┤f│
            // │e│ field2       ││ field3       │e│
            // │e│              ││              │e│
            // │d│              ││              │d│
            // │b│              ││              │b│
            // │2│              ││              │3│
            // │ ├──────────────┴┴──────────────┤ │
            // │ │             status           │ │
            // └─┴──────────────────────────────┴─┘

            fieldWidth = (width - 2 * feedbackWidth - fieldHSeparation) / 2;
            fieldVSeparation = (height - 2 * fieldHeight) / 4;

            field[0].area.x = feedbackWidth;
            field[0].area.y = 0;
            field[0].area.w = fieldWidth;
            field[0].area.h = fieldHeight;

            field[1].area.x = feedbackWidth + fieldWidth + fieldHSeparation;
            field[1].area.y = 0;
            field[1].area.w = fieldWidth;
            field[1].area.h = fieldHeight;

            field[2].area.x = feedbackWidth;
            field[2].area.y = fieldHeight + fieldVSeparation;
            field[2].area.w = fieldWidth;
            field[2].area.h = fieldHeight;

            field[3].area.x = feedbackWidth + fieldWidth + fieldHSeparation;
            field[3].area.y = fieldHeight + fieldVSeparation;
            field[3].area.w = fieldWidth;
            field[3].area.h = fieldHeight;

            field[0].content[0] = FC_POWER;
            field[1].content[0] = FC_HEARTRATE;
            field[2].content[0] = FC_CADENCE;
            field[3].content[0] = FC_HEARTRATE;

            field[0].content[1] = FC_SPEED;
            field[1].content[1] = FC_DISTANCE;
            field[2].content[1] = FC_ALTGAIN;
            field[3].content[1] = FC_ALTGAIN;

            field[0].content[2] = FC_BATTERY;
            field[1].content[2] = FC_BATTERY_POWER;
            field[2].content[2] = FC_BATTERY_HEARTRATE;
            field[3].content[2] = FC_BATTERY_HEARTRATE;
    */

    // ┌─┬──────────────────────────────┬─┐
    // │f│ field0 / clock               │f│
    // │e│                              │e│
    // │e│                              │e│
    // │d│                              │d│
    // │b│                              │b│
    // │0│                              │1│
    // ├─┼──────────────────────────────┼─┤
    // │f├──────────────┬┬──────────────┤f│
    // │e│ field1       ││ field2       │e│
    // │e│              ││              │e│
    // │d│              ││              │d│
    // │b├──────────────┴┴──────────────┤b│
    // │2├──────────────────────────────┤3│
    // │ │             status           │ │
    // └─┴──────────────────────────────┴─┘

    fieldWidth = (width - 2 * feedbackWidth - fieldHSeparation) / 2;
    fieldVSeparation = 9;

    field[0].area.x = feedbackWidth;
    field[0].area.y = 0;
    field[0].area.w = width - 2 * feedbackWidth;
    field[0].area.h = 62;  // height - 2 * fieldVSeparation - fieldHeight - statusIconSize;

    field[1].area.x = feedbackWidth;
    field[1].area.y = field[0].area.h + fieldVSeparation;
    field[1].area.w = fieldWidth;
    field[1].area.h = fieldHeight;

    field[2].area.x = feedbackWidth + fieldWidth + fieldHSeparation;
    field[2].area.y = field[1].area.y;
    field[2].area.w = fieldWidth;
    field[2].area.h = fieldHeight;

    field[0].content[0] = FC_POWER;
    field[1].content[0] = FC_CADENCE;
    field[2].content[0] = FC_HEARTRATE;

    field[0].content[1] = FC_POWER;
    field[1].content[1] = FC_RANGE;
    field[2].content[1] = FC_BATTERY_VESC;

    field[0].content[2] = FC_SPEED;
    field[1].content[2] = FC_DISTANCE;
    field[2].content[2] = FC_ALTGAIN;

    field[0].content[3] = FC_BATTERY;
    field[1].content[3] = FC_BATTERY_POWER;
    field[2].content[3] = FC_BATTERY_HEARTRATE;

    fieldFont = (uint8_t *)u8g2_font_logisoso32_tr;
    smallFont = (uint8_t *)u8g2_font_logisoso18_tr;
    timeFont = (uint8_t *)u8g2_font_logisoso42_tr;
    timeFontHeight = 42;
    dateFont = (uint8_t *)u8g2_font_fur14_tn;
    dateFontHeight = dateFontHeight;
    labelFont = (uint8_t *)u8g2_font_bytesize_tr;
    labelFontHeight = 12;

    field[0].font = (uint8_t *)u8g2_font_logisoso50_tr;
    field[0].labelFont = labelFont;
    field[0].smallFont = (uint8_t *)u8g2_font_logisoso32_tr;
    field[0].smallFontWidth = 20;

    for (uint8_t i = 1; i < numFields; i++) {
        field[i].font = fieldFont;
        field[i].labelFont = labelFont;
        field[i].smallFont = smallFont;
        field[i].smallFontWidth = 13;
    }

    feedback[0].x = 0;
    feedback[0].y = 0;
    feedback[0].w = feedbackWidth;
    feedback[0].h = height / 2;

    feedback[1].x = width - feedbackWidth;
    feedback[1].y = 0;
    feedback[1].w = feedbackWidth;
    feedback[1].h = height / 2;

    feedback[2].x = 0;
    feedback[2].y = height / 2;
    feedback[2].w = feedbackWidth;
    feedback[2].h = height / 2;

    feedback[3].x = width - feedbackWidth;
    feedback[3].y = height / 2;
    feedback[3].w = feedbackWidth;
    feedback[3].h = height / 2;

    statusArea = Area(feedbackWidth,
                      height - statusIconSize,
                      width - feedbackWidth * 2,
                      statusIconSize);

    clockArea = field[0].area;
}

Lcd::~Lcd() {
    delete device;
}

void Lcd::setup(uint8_t backlightPin) {
    Display::setup();
    Arduino_Canvas::begin();
    if (aquireMutex()) {
        fillScreen(bg);
        releaseMutex();
    }
    if (INT8_MAX < backlightPin)
        log_e("pin out of range");
    else
        this->backlightPin = (int8_t)backlightPin;
    pinMode(backlightPin, OUTPUT);
    backlight(backlightState);
    // device->begin();
    setTextWrap(false);
    setTextColor(fg);
    if (aquireMutex()) {
        splash(false);
        sendBuffer();
        enabled = false;
        releaseMutex();
        if (!queue([this]() { enabled = true; }, 2000)) enabled = true;
    }
    // for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
    //     log_i("field %d:    %3d %3d %3d %3d", i, field[i].area.x, field[i].area.y, field[i].area.w, field[i].area.h);
    // for (uint8_t i = 0; i < sizeof(feedback) / sizeof(feedback[0]); i++)
    //     log_i("feedback %d: %3d %3d %3d %3d", i, feedback[i].x, feedback[i].y, feedback[i].w, feedback[i].h);
    // log_i("status:     %3d %3d %3d %3d", statusArea.x, statusArea.y, statusArea.w, statusArea.h);
    // log_i("clock:      %3d %3d %3d %3d", clockArea.x, clockArea.y, clockArea.w, clockArea.h);
}

void Lcd::writePixelPreclipped(int16_t x, int16_t y, uint16_t color) {
    // log_i("%d %d %d", x, y, color);
    if (!clip.contains(x, y))
        return;
    Arduino_Canvas::writePixelPreclipped(x, y, color);
}

void Lcd::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (!_ordered_in_range(x, clip.x, clip.x + clip.w))
        return;

    if (y < clip.y) {
        if (y + h < clip.y) return;
        h -= clip.y - y;
        y = clip.y;
    }

    int16_t clipBottom = clip.y + clip.h;
    if (clipBottom < y + h) {
        if (clipBottom < y) return;
        h = clipBottom - y;
    }

    Arduino_Canvas::writeFastVLine(x, y, h, color);
}

void Lcd::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (!_ordered_in_range(y, clip.y, clip.y + clip.h))
        return;

    if (x < clip.x) {
        if (x + w < clip.x) return;
        w -= clip.x - x;
        x = clip.x;
    }

    int16_t clipRight = clip.x + clip.w;
    if (clipRight < x + w) {
        if (clipRight < x) return;
        w = clipRight - x;
    }

    Arduino_Canvas::writeFastHLine(x, y, w, color);
}

void Lcd::writeFillRectPreclipped(int16_t x, int16_t y,
                                  int16_t w, int16_t h, uint16_t color) {
    Area a = Area(x, y, w, h);

    if (!clip.touches(&a))
        return;

    if (clip.contains(&a)) {
        Arduino_Canvas::writeFillRectPreclipped(x, y, w, h, color);
        return;
    }

    // log_i("before %d %d %d %d", a.x, a.y, a.w, a.h);
    // a.clipTo(&clip);
    // log_i("after  %d %d %d %d", a.x, a.y, a.w, a.h);
    Arduino_Canvas::writeFillRectPreclipped(a.x, a.y, a.w, a.h, color);
}

void Lcd::fillScreen(uint16_t color) {
    startWrite();
    writeFillRectPreclipped(0, 0, Arduino_Canvas::width(), Arduino_Canvas::height(), color);
    endWrite();
}

void Lcd::setClip(int16_t x, int16_t y, int16_t w, int16_t h) {
    // if (x != 0 || y != 0 || w != Arduino_Canvas::width() || h != Arduino_Canvas::height()) {
    //     log_i("%d %d %d %d", x, y, w, h);
    //     logAreas(&clip, "clip");
    // }
    clip = Area(x, y, w, h);
}

size_t Lcd::write(uint8_t c) {
    // log_i("%c", c);
    return Arduino_Canvas::write(c);
}

void Lcd::setFont(const uint8_t *font) {
    if (nullptr == font) {
        log_e("font is null");
        return;
    }
    Arduino_Canvas::setFont(font);
}

void Lcd::setColor(uint16_t color) {
    Display::setColor(color);
    setTextColor(color);
}

void Lcd::fill(const Area *a, uint16_t color, bool send) {
    if (board.otaMode) return;
    fillUnrestricted(a, color, send);
}

void Lcd::fillUnrestricted(const Area *a, uint16_t color, bool send) {
    // log_i("%d %d %d %d %d", a->x, a->y, a->w, a->h, color);
    // logAreas((Area *)a, "fill");
    if (send && !aquireMutex()) return;
    // Arduino_Canvas::fillRect(a->x, a->y, a->w, a->h, color);
    Arduino_Canvas::startWrite();
    writeFillRectPreclipped(a->x, a->y, a->w, a->h, color);
    Arduino_Canvas::endWrite();
    if (!send) return;
    sendBuffer();
    releaseMutex();
}

void Lcd::setCursor(int16_t x, int16_t y) {
    // log_i("%d %d", x, y);
    Arduino_Canvas::setCursor(x, y);
}

void Lcd::sendBuffer() {
    Arduino_Canvas::flush();
}

void Lcd::drawXBitmap(int16_t x,
                      int16_t y,
                      int16_t w,
                      int16_t h,
                      const uint8_t *bitmap,
                      uint16_t color,
                      bool send) {
    // log_i("%d %d %d %d %d", x, y, w, h, color);
    Area a = Area(x, y, w, h);
    // logAreas(&a, "clip");
    if (send && !aquireMutex()) return;
    Arduino_Canvas::drawXBitmap(x, y, bitmap, w, h, color);
    if (!send) return;
    sendBuffer();
    releaseMutex();
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
    setCursor(a->x, a->y + timeFontHeight + 2);
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

void Lcd::updateStatus(bool forceRedraw) {
    // 4th of 5 evenly spaced, equally sized icons
    static Area icon = Area(statusArea.x + ((statusArea.w - statusIconSize) / 4) * 3,
                            statusArea.y, statusIconSize, statusIconSize);
    static const uint8_t backlightXbm[] = {
        0x00, 0x00, 0x40, 0x00, 0x44, 0x04, 0x08, 0x02, 0xe0, 0x00, 0xb0, 0x01,
        0x16, 0x0d, 0xb0, 0x01, 0xe0, 0x00, 0x08, 0x02, 0x44, 0x04, 0x40, 0x00,
        0x00, 0x00, 0x00, 0x00};
    static int16_t lastBacklightState = -1;
    if ((lastBacklightState != backlightState || forceRedraw) && aquireMutex()) {
        fill(&icon, bg, false);
        if (backlightState) drawXBitmap(icon.x, icon.y, icon.w, icon.h, backlightXbm, fg, false);
        sendBuffer();
        releaseMutex();
        lastBacklightState = (int16_t)backlightState;
    }
    Display::updateStatus(forceRedraw);
}

// the return value indicates whether the event should propagate
bool Lcd::onTouchEvent(Touch::Pad *pad, Touch::Event event) {
    if (1 == pad->index && Touch::Event::tripleTouch == event) {  // top right
        if (backlightState) {
            backlight(0);
            fg = BLACK;
            bg = WHITE;
        } else {
            backlight(UINT8_MAX);
            fg = WHITE;
            bg = BLACK;
        }
        setTextColor(fg);
        fillScreen(bg);
        updateStatus(true);
        displayFieldValues(true);
        return false;  // do not propagate
    }
    return Display::onTouchEvent(pad, event);
}

void Lcd::backlight(uint8_t state) {
    backlightState = state;
    if (this->backlightPin < 0) {
        log_e("pin not set");
        return;
    }
    // log_d("setting backlight: %d", backlightState);
    digitalWrite(this->backlightPin, backlightState);
}

void Lcd::diag(bool send) {
    if (send && !aquireMutex()) return;
    fill(&field[0].area, rgb888to565(255, 0, 0), false);
    fill(&field[1].area, rgb888to565(0, 255, 0), false);
    fill(&field[2].area, rgb888to565(0, 0, 255), false);
    fill(&feedback[0], rgb888to565(255, 255, 0), false);
    fill(&feedback[1], rgb888to565(255, 0, 255), false);
    fill(&feedback[2], rgb888to565(0, 128, 128), false);
    fill(&feedback[3], rgb888to565(255, 128, 0), false);
    fill(&statusArea, rgb888to565(128, 0, 128), false);
    // printField(0, "1234");
    // printField(1, "567");
    // printField(2, "890");
    if (send) {
        sendBuffer();
        releaseMutex();
    }
}

uint16_t Lcd::lockedFg() { return WHITE; }
uint16_t Lcd::lockedBg() { return RED; }
uint16_t Lcd::unlockedFg() { return WHITE; }
uint16_t Lcd::unlockedBg() { return DARKGREEN; }
uint16_t Lcd::tareFg() { return YELLOW; }
uint16_t Lcd::tareBg() { return color565(0, 0, 127); }     // dark blue
uint16_t Lcd::pasFg() { return color565(255, 180, 127); }  // orange
uint16_t Lcd::pasBg() { return color565(0, 0, 127); }      // dark blue