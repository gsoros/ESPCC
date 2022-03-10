#ifndef __oled_h
#define __oled_h

#include <Arduino.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>

//#include "board.h"

class Oled {
   public:
    // U8G2_SH1106_128X64_VCOMH0_F_HW_I2C *oled;
    // U8G2_SH1106_128X64_WINSTAR_F_HW_I2C *oled;
    U8G2_SH1106_128X64_NONAME_F_HW_I2C *oled;

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
        // Serial.printf("Secs: %d\n", gps->time.second());
        // return;
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
    }

    void displaySatellites(uint32_t satellites) {
        // Serial.printf("Sats: %d\n", satellites);
        // return;
        oled->clearBuffer();
        oled->setCursor(0, 82);
        oled->printf("%03d", satellites);
        oled->sendBuffer();
    }

    void setContrast(uint8_t contrast) {
        oled->setContrast(contrast);
    }
};

#endif