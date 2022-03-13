#ifndef __oled_h
#define __oled_h

#include <U8g2lib.h>
#include <TinyGPS++.h>

class Oled {
   public:
    Oled() {
        // oled = new U8G2_SH1106_128X64_VCOMH0_F_HW_I2C(
        // oled = new U8G2_SH1106_128X64_WINSTAR_F_HW_I2C(
        oled = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(
            U8G2_R1,
            U8X8_PIN_NONE,
            GPIO_NUM_14,
            GPIO_NUM_12);
    }

    void setup() {
        oled->begin();
        oled->setFont(u8g2_font_logisoso32_tn);
    }

    void displayGps(TinyGPSPlus *gps) {
        if (!lock()) return;
        oled->clearBuffer();
        oled->setCursor(0, 32);
        oled->printf("%02d%d",
                     gps->time.hour(),
                     gps->time.minute() / 10);
        oled->setCursor(0, 82);
        oled->printf("%d%02d",
                     gps->time.minute() % 10,
                     gps->time.second());
        oled->setCursor(0, 128);
        oled->printf("%d%02d",
                     gps->date.month() % 10,
                     gps->date.day());
        oled->sendBuffer();
        release();
    }

    void displaySatellites(uint32_t satellites) {
        if (!lock()) return;
        oled->clearBuffer();
        oled->setCursor(0, 82);
        oled->printf("%03d", satellites);
        oled->sendBuffer();
        release();
    }

    void onTouchEvent(uint8_t index, uint8_t event) {
        // if (ATOLL_TOUCH_START != event) return;
        if (!lock()) return;
        oled->clearBuffer();
        oled->setCursor(0, 82);
        oled->printf("%d %d", index, event);
        oled->sendBuffer();
        release();
    }

    bool setContrast(uint8_t contrast) {
        if (!lock()) return false;
        oled->setContrast(contrast);
        release();
        return true;
    }

   protected:
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    // U8G2_SH1106_128X64_VCOMH0_F_HW_I2C *oled;
    // U8G2_SH1106_128X64_WINSTAR_F_HW_I2C *oled;
    U8G2_SH1106_128X64_NONAME_F_HW_I2C *oled;

    bool lock(uint32_t timeout = portMAX_DELAY) {
        if (xSemaphoreTake(mutex, (TickType_t)timeout) == pdTRUE)
            return true;
        log_e("Could not aquire lock");
        return false;
    }

    void release() {
        xSemaphoreGive(mutex);
    }
};

#endif