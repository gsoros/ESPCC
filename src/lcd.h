#ifndef __lcd_h
#define __lcd_h

#include <U8g2lib.h>

//#include <Arduino_GFX_Library.h>
//#include "databus/Arduino_HWSPI.h"
//#include "databus/Arduino_ESP32SPI.h"
//#include "display/Arduino_SSD1283A.h"
#include "canvas/Arduino_Canvas.h"

#include "display.h"
#include "atoll_time.h"
#include "touch.h"

class Lcd : public Display, public Arduino_Canvas {
   public:
    Area clip = Area();
    uint8_t fieldHSeparation = 2;
    int8_t backlightPin = -1;
    uint8_t backlightState = 1;

    Lcd(Arduino_GFX *device,
        uint8_t width = 130,
        uint8_t height = 130,
        uint8_t feedbackWidth = 3,
        uint8_t fieldHeight = 36,
        SemaphoreHandle_t *mutex = nullptr)
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

        field[0].content[1] = FC_SPEED;
        field[1].content[1] = FC_DISTANCE;
        field[2].content[1] = FC_ALTGAIN;

        field[0].content[2] = FC_BATTERY;
        field[1].content[2] = FC_BATTERY_POWER;
        field[2].content[2] = FC_BATTERY_HEARTRATE;

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

    virtual ~Lcd();

    virtual void setup(uint8_t backlightPin) {
        if (INT8_MAX < backlightPin)
            log_e("pin out of range");
        else
            this->backlightPin = (int8_t)backlightPin;
        pinMode(backlightPin, OUTPUT);
        backlight(backlightState);
        Arduino_Canvas::begin();
        // device->begin();
        Display::setup();
        setTextWrap(false);
        setTextColor(fg);
        if (aquireMutex()) {
            fillScreen(bg);
            releaseMutex();
        }
        diag();
        delay(5000);
        if (aquireMutex()) {
            fillScreen(bg);
            releaseMutex();
        }
        splash();

        for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
            log_i("field %d:    %3d %3d %3d %3d", i, field[i].area.x, field[i].area.y, field[i].area.w, field[i].area.h);
        for (uint8_t i = 0; i < sizeof(feedback) / sizeof(feedback[0]); i++)
            log_i("feedback %d: %3d %3d %3d %3d", i, feedback[i].x, feedback[i].y, feedback[i].w, feedback[i].h);
        log_i("status:     %3d %3d %3d %3d", statusArea.x, statusArea.y, statusArea.w, statusArea.h);
        log_i("clock:      %3d %3d %3d %3d", clockArea.x, clockArea.y, clockArea.w, clockArea.h);
    }

    virtual void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {
        // log_i("%d %d %d", x, y, color);
        if (!clip.contains(x, y))
            return;
        Arduino_Canvas::writePixelPreclipped(x, y, color);
    }

    virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
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

    virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
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

    virtual void writeFillRectPreclipped(int16_t x, int16_t y,
                                         int16_t w, int16_t h, uint16_t color) override {
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

    virtual void fillScreen(uint16_t color) {
        startWrite();
        writeFillRectPreclipped(0, 0, Arduino_Canvas::width(), Arduino_Canvas::height(), color);
        endWrite();
    }

    virtual void setClip(int16_t x, int16_t y, int16_t w, int16_t h) override {
        // if (x != 0 || y != 0 || w != Arduino_Canvas::width() || h != Arduino_Canvas::height()) {
        //     log_i("%d %d %d %d", x, y, w, h);
        //     logAreas(&clip, "clip");
        // }
        clip = Area(x, y, w, h);
    }

    virtual size_t write(uint8_t c) {
        // log_i("%c", c);
        return Arduino_Canvas::write(c);
    }

    virtual void setFont(const uint8_t *font) override {
        if (nullptr == font) {
            log_e("font is null");
            return;
        }
        Arduino_Canvas::setFont(font);
    }

    virtual void fill(const Area *a, uint16_t color, bool send = true) override;
    virtual void fillUnrestricted(const Area *a, uint16_t color, bool send = true) {
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

    virtual void setCursor(int16_t x, int16_t y) override {
        // log_i("%d %d", x, y);
        Arduino_Canvas::setCursor(x, y);
    }

    virtual void sendBuffer() override {
        Arduino_Canvas::flush();
    }

    virtual void drawXBitmap(int16_t x,
                             int16_t y,
                             int16_t w,
                             int16_t h,
                             const uint8_t *bitmap,
                             uint16_t color,
                             bool send = true) override {
        // log_i("%d %d %d %d %d", x, y, w, h, color);
        Area a = Area(x, y, w, h);
        // logAreas(&a, "clip");
        if (send && !aquireMutex()) return;
        Arduino_Canvas::drawXBitmap(x, y, bitmap, w, h, color);
        if (!send) return;
        sendBuffer();
        releaseMutex();
    }

    virtual void clock(bool send = true, bool clear = false, int8_t skipFieldIndex = -1) override;

    virtual void updateStatus() override {
        // 4th of 5 evenly spaced, equally sized icons
        static Area icon = Area(statusArea.x + ((statusArea.w - statusIconSize) / 4) * 3,
                                statusArea.y, statusIconSize, statusIconSize);
        static const uint8_t backlightXbm[] = {
            0x00, 0x00, 0x40, 0x00, 0x44, 0x04, 0x08, 0x02, 0xe0, 0x00, 0xb0, 0x01,
            0x16, 0x0d, 0xb0, 0x01, 0xe0, 0x00, 0x08, 0x02, 0x44, 0x04, 0x40, 0x00,
            0x00, 0x00, 0x00, 0x00};
        static int16_t lastBacklightState = -1;
        if (lastBacklightState != backlightState && aquireMutex()) {
            fill(&icon, bg, false);
            if (backlightState) drawXBitmap(icon.x, icon.y, icon.w, icon.h, backlightXbm, fg, false);
            sendBuffer();
            releaseMutex();
            lastBacklightState = (int16_t)backlightState;
        }
        Display::updateStatus();
    }

    // the return value indicates whether the event should propagate
    virtual bool onTouchEvent(Touch::Pad *pad, Touch::Event event) override {
        if (3 == pad->index && Touch::Event::longTouch == event) {
            backlight(!backlightState);
            return false;  // do not propagate
        }
        return Display::onTouchEvent(pad, event);
    }

    void backlight(uint8_t state) {
        backlightState = state;
        if (this->backlightPin < 0) {
            log_e("pin not set");
            return;
        }
        log_i("setting backlight: %d", backlightState);
        digitalWrite(this->backlightPin, backlightState);
    }

    void diag() {
        if (!aquireMutex()) return;
        fill(&field[0].area, rgb888to565(255, 0, 0), false);
        fill(&field[1].area, rgb888to565(0, 255, 0), false);
        fill(&field[2].area, rgb888to565(0, 0, 255), false);
        fill(&feedback[0], rgb888to565(255, 255, 0), false);
        fill(&feedback[1], rgb888to565(255, 0, 255), false);
        fill(&feedback[2], rgb888to565(255, 255, 128), false);
        fill(&feedback[3], rgb888to565(255, 128, 0), false);
        fill(&statusArea, rgb888to565(128, 0, 128), false);
        // printField(0, "1234");
        // printField(1, "567");
        // printField(2, "890");
        sendBuffer();
        releaseMutex();
    }

   protected:
    Arduino_GFX *device;
};

#endif