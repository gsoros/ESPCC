#ifndef __oled_h
#define __oled_h

#include <U8g2lib.h>
#include <TinyGPS++.h>

#include "touch.h"
#include "atoll_task.h"

class Oled : public Atoll::Task {
    struct Area {
        uint8_t x;  // top left x
        uint8_t y;  // top left y
        uint8_t w;  // width
        uint8_t h;  // height
    };

   public:
    const uint8_t width = 64;
    const uint8_t height = 128;
    const uint8_t feedbackWidth = 3;
    const uint8_t fieldHeight = 32;
    Area fields[3];
    Area feedback[4];

    Oled() {
        // oled = new U8G2_SH1106_128X64_VCOMH0_F_HW_I2C(
        // oled = new U8G2_SH1106_128X64_WINSTAR_F_HW_I2C(
        device = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(
            U8G2_R1,        // rotation 90˚
            U8X8_PIN_NONE,  // reset pin none
            GPIO_NUM_14,    // sck pin
            GPIO_NUM_12);   // sda pin

        const uint8_t fieldWidth = width - 2 * feedbackWidth;
        const uint8_t fieldVSeparation = (height - 3 * fieldHeight) / 2;

        fields[0].x = feedbackWidth;
        fields[0].y = 0;
        fields[0].w = fieldWidth;
        fields[0].h = fieldHeight;

        fields[1].x = feedbackWidth;
        fields[1].y = fieldHeight + fieldVSeparation;
        fields[1].w = fieldWidth;
        fields[1].h = fieldHeight;

        fields[2].x = feedbackWidth;
        fields[2].y = height - fieldHeight;
        fields[2].w = fieldWidth;
        fields[2].h = fieldHeight;

        feedback[0].x = 0;
        feedback[0].y = 0;
        feedback[0].w = feedbackWidth;
        feedback[0].h = height / 2;

        feedback[1].x = 0;
        feedback[1].y = height / 2;
        feedback[1].w = feedbackWidth;
        feedback[1].h = height / 2;

        feedback[2].x = width - feedbackWidth;
        feedback[2].y = 0;
        feedback[2].w = feedbackWidth;
        feedback[2].h = height / 2;

        feedback[3].x = width - feedbackWidth;
        feedback[3].y = height / 2;
        feedback[3].w = feedbackWidth;
        feedback[3].h = height / 2;
    }

    virtual ~Oled() {
        delete device;
    }

    void setup() {
        device->begin();
        device->setFont(u8g2_font_logisoso32_tn);
    }

    void loop();

    void printfField(uint8_t positionIndex, bool send, const char *format, ...) {
        assert(positionIndex < sizeof(fields) / sizeof(fields[0]));
        if (send && !aquireMutex()) return;
        char out[4];
        va_list argp;
        va_start(argp, format);
        vsnprintf(out, 4, format, argp);
        va_end(argp);
        Area *a = &fields[positionIndex];
        // log_i("%d(%d, %d, %d, %d) %s", positionIndex, a->x, a->y, a->w, a->h, out);
        fill(a, 0, false);
        device->setCursor(a->x, a->y + a->h);
        uint8_t oldColor = device->getDrawColor();
        device->setDrawColor(1);
        device->print(out);
        device->setDrawColor(oldColor);
        if (!send) return;
        device->sendBuffer();
        releaseMutex();
    }

    void fill(Area *a, uint8_t color, bool send) {
        if (send && !aquireMutex()) return;
        uint8_t oldColor = device->getDrawColor();
        device->setDrawColor(color);
        device->drawBox(a->x, a->y, a->w, a->h);
        device->setDrawColor(oldColor);
        if (!send) return;
        device->sendBuffer();
        releaseMutex();
    }

    void
    displayGps(TinyGPSPlus *gps) {
        if (!aquireMutex()) return;
        printfField(0, false, "%02d%d",
                    gps->time.hour(),
                    gps->time.minute() / 10);
        printfField(1, false, "%d%02d",
                    gps->time.minute() % 10,
                    gps->time.second());
        printfField(2, false, "%d%02d",
                    gps->date.month() % 10,
                    gps->date.day());
        device->sendBuffer();
        releaseMutex();
    }

    void displaySatellites(uint32_t satellites) {
        printfField(2, true, "%03d", satellites);
    }

    void onTouchEvent(TouchPad *pad, uint8_t event);

    bool setContrast(uint8_t contrast) {
        if (!aquireMutex()) return false;
        device->setContrast(contrast);
        releaseMutex();
        return true;
    }

   protected:
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    U8G2 *device;

    bool aquireMutex(uint32_t timeout = portMAX_DELAY) {
        if (xSemaphoreTake(mutex, (TickType_t)timeout) == pdTRUE)
            return true;
        log_e("Could not aquire mutex");
        return false;
    }

    void releaseMutex() {
        xSemaphoreGive(mutex);
    }
};

#endif