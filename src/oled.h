#ifndef __oled_h
#define __oled_h

#include <U8g2lib.h>

//#include "touch.h"  // avoid "NUM_PADS redefined"
#include "atoll_time.h"
//#include "atoll_oled.h"
#include "display.h"

class Oled : public Display {
   public:
    Oled(U8G2 *device,
         uint8_t width = 64,
         uint8_t height = 128,
         uint8_t feedbackWidth = 3,
         uint8_t fieldHeight = 32)
        : Display(width, height, feedbackWidth, fieldHeight),
          device(device) {
        // ┌──────┬───────────────┬──────┐
        // │feedb0│ field0        │feedb2│
        // │      │            c  │      │
        // │      ├───────────────┤      │
        // │      ├────────────l──┤      │
        // │      │ field1        │      │
        // │      │            o  │      │
        // ├──────┼───────────────┼──────┤
        // │feedb1├────────────c──┤feedb3│
        // │      │ field2        │      │
        // │      │            k  │      │
        // │      ├───────────────┤      │
        // │      ├───────────────┤      │
        // │      │ status        │      │
        // │      │               │      │
        // └──────┴───────────────┴──────┘

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

    virtual ~Oled();

    void setup() {
        device->begin();
        Display::setup();
        device->setFontMode(0);  //     // draw gylph bg
        // device->setFontMode(1);      // do not draw gylph bg
        device->setBitmapMode(0);  //   // draw bitmap bg
        // device->setBitmapMode(1);    // do not draw bitmap bg
        // log_i("setting draw color to %d", colorIndex(fg));
        device->setDrawColor(colorIndex(fg));
        splash();
    }

    virtual size_t write(uint8_t c) {
        return device->write(c);
    }

    inline uint16_t nonNeg(int16_t x) {
        if (x < 0) {
            log_e("negative coords not supported");
            x = 0;
        }
        return (uint16_t)x;
    }

    uint8_t colorIndex(uint16_t color) {
        // log_i("color: %d -> %d", color, 0 < color ? 1 : 0);
        return 0 < color ? 1 : 0;
    }

    virtual void setFont(const uint8_t *font) {
        if (nullptr == font) {
            log_e("font is null");
            return;
        }
        device->setFont(font);
    }

    virtual void fill(const Area *a, uint16_t color, bool send = true) {
        // log_i("color: %d", color);
        if (send && !aquireMutex()) return;
        uint8_t c = device->getDrawColor();
        device->setDrawColor(colorIndex(color));
        device->drawBox(nonNeg(a->x), nonNeg(a->y), nonNeg(a->w), nonNeg(a->h));
        device->setDrawColor(c);
        if (!send) return;
        sendBuffer();
        releaseMutex();
    }

    virtual void setClip(int16_t x, int16_t y, int16_t w, int16_t h) {
        device->setClipWindow(nonNeg(x), nonNeg(y), nonNeg(x + w), nonNeg(y + h));
    }

    virtual void setCursor(int16_t x, int16_t y) {
        device->setCursor(nonNeg(x), nonNeg(y));
    }

    virtual void sendBuffer() {
        device->sendBuffer();
    }

    virtual void drawXBitmap(int16_t x,
                             int16_t y,
                             int16_t w,
                             int16_t h,
                             const uint8_t *bitmap,
                             uint16_t color,
                             bool send = true) {
        // log_i("%d %d %d %d %d", x, y, w, h, color);
        // log_i("%d %d %d %d %d", nonNeg(x), nonNeg(y), nonNeg(w), nonNeg(h), colorIndex(color));
        if (send && !aquireMutex()) return;
        uint8_t c = device->getDrawColor();
        device->setDrawColor(colorIndex(color));
        device->drawXBM(nonNeg(x), nonNeg(y), nonNeg(w), nonNeg(h), bitmap);
        device->setDrawColor(c);
        if (!send) return;
        sendBuffer();
        releaseMutex();
    }

    virtual void clock(bool send = true, bool clear = false, int8_t skipFieldIndex = -1) {
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

   protected:
    U8G2 *device;
};

#endif