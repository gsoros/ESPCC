#ifndef __lcd_h
#define __lcd_h

#include <U8g2lib.h>

//#include <Arduino_GFX_Library.h>
//#include "databus/Arduino_HWSPI.h"
//#include "databus/Arduino_ESP32SPI.h"
//#include "display/Arduino_SSD1283A.h"
#include "canvas/Arduino_Canvas.h"

#include "atoll_time.h"
#include "display.h"

class Lcd : public Display, public Arduino_Canvas {
   public:
    Area clip = Area();

    Lcd(Arduino_GFX *device,
        uint8_t width = 130,
        uint8_t height = 130,
        uint8_t feedbackWidth = 3,
        uint8_t fieldHeight = 32,
        SemaphoreHandle_t *mutex = nullptr)
        : Display(width, height, feedbackWidth, fieldHeight, mutex),
          Arduino_Canvas(width, height, device, 0, 0) {
        // ┌──────┬────────────────────────┬──────┐
        // │feedb0│ field0                 │feedb2│
        // │      │            c           │      │
        // │      ├────────────────────────┤      │
        // │      ├────────────l───────────┤      │
        // │      │ field1                 │      │
        // │      │            o           │      │
        // ├──────┼────────────────────────┼──────┤
        // │feedb1├────────────c───────────┤feedb3│
        // │      │ field2                 │      │
        // │      │            k           │      │
        // │      ├────────────────────────┤      │
        // │      ├────────────────────────┤      │
        // │      │ status                 │      │
        // │      │                        │      │
        // └──────┴────────────────────────┴──────┘

        field[0].area.x = feedbackWidth;
        field[0].area.y = 0;
        field[0].area.w = fieldWidth;
        field[0].area.h = fieldHeight;

        field[1].area.x = feedbackWidth;
        field[1].area.y = fieldHeight + fieldVSeparation;
        field[1].area.w = fieldWidth;
        field[1].area.h = fieldHeight;

        field[2].area.x = feedbackWidth;
        field[2].area.y = 2 * fieldHeight + 2 * fieldVSeparation;
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

        feedback[0].x = 0;
        feedback[0].y = 0;
        feedback[0].w = feedbackWidth;
        feedback[0].h = height / 2;
        // feedback[0].invert = true;

        feedback[1].x = 0;
        feedback[1].y = height / 2;
        feedback[1].w = feedbackWidth;
        feedback[1].h = height / 2;

        feedback[2].x = width - feedbackWidth;
        feedback[2].y = 0;
        feedback[2].w = feedbackWidth;
        feedback[2].h = height / 2;
        // feedback[2].invert = true;

        feedback[3].x = width - feedbackWidth;
        feedback[3].y = height / 2;
        feedback[3].w = feedbackWidth;
        feedback[3].h = height / 2;

        statusArea = Area(feedbackWidth,
                          height - statusIconSize,
                          fieldWidth,
                          statusIconSize);

        clockArea.x = field[0].area.x;
        clockArea.y = field[0].area.y;
        clockArea.w = fieldWidth;
        clockArea.h = numFields * fieldHeight + (numFields - 1) * fieldVSeparation;

        fieldFont = (uint8_t *)u8g2_font_logisoso32_tr;
        fieldDigitFont = (uint8_t *)u8g2_font_logisoso32_tn;
        smallFont = (uint8_t *)u8g2_font_logisoso18_tr;
        timeFont = (uint8_t *)u8g2_font_logisoso32_tn;  // font for displaying the time
        timeFontHeight = 32;                            //
        dateFont = (uint8_t *)u8g2_font_fur14_tn;       // font for displaying the date
        dateFontHeight = dateFontHeight;                //
        labelFont = (uint8_t *)u8g2_font_bytesize_tr;   // font for displaying the field labels
        labelFontHeight = 12;
    }

    virtual ~Lcd();

    virtual void setup(uint8_t backlightPin) {
        pinMode(backlightPin, OUTPUT);
        digitalWrite(backlightPin, HIGH);
        Arduino_Canvas::begin();
        // device->begin();
        Display::setup();
        setTextWrap(false);
        setTextColor(fg);
        fillScreen(bg);
        splash();

        for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
            log_i("field %d:    %d %d %d %d", i, field[i].area.x, field[i].area.y, field[i].area.w, field[i].area.h);
        for (uint8_t i = 0; i < sizeof(feedback) / sizeof(feedback[0]); i++)
            log_i("feedback %d: %d %d %d %d", i, feedback[i].x, feedback[i].y, feedback[i].w, feedback[i].h);
        log_i("status:      %d %d %d %d", statusArea.x, statusArea.y, statusArea.w, statusArea.h);
        log_i("clock:       %d %d %d %d", clockArea.x, clockArea.y, clockArea.w, clockArea.h);
    }

    virtual void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {
        // log_i("%d %d %d", x, y, color);
        if (!clip.contains(x, y)) return;
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

   protected:
    Arduino_GFX *device;
};

#endif