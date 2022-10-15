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
    uint8_t backlightState = UINT8_MAX;

    Lcd(Arduino_GFX *device,
        uint8_t width = 130,
        uint8_t height = 130,
        uint8_t feedbackWidth = 3,
        uint8_t fieldHeight = 36,
        SemaphoreHandle_t *mutex = nullptr);

    virtual ~Lcd();

    virtual void setup(uint8_t backlightPin);
    virtual void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override;
    virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;
    virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;
    virtual void writeFillRectPreclipped(int16_t x, int16_t y,
                                         int16_t w, int16_t h, uint16_t color) override;
    virtual void fillScreen(uint16_t color);
    virtual void setClip(int16_t x, int16_t y, int16_t w, int16_t h);
    virtual size_t write(uint8_t c);
    virtual void setFont(const uint8_t *font) override;
    virtual void setColor(uint16_t color);
    virtual void fill(const Area *a, uint16_t color, bool send = true) override;
    virtual void fillUnrestricted(const Area *a, uint16_t color, bool send = true);
    virtual void setCursor(int16_t x, int16_t y) override;
    virtual void sendBuffer() override;
    virtual void drawXBitmap(int16_t x,
                             int16_t y,
                             int16_t w,
                             int16_t h,
                             const uint8_t *bitmap,
                             uint16_t color,
                             bool send = true) override;
    virtual void clock(bool send = true, bool clear = false, int8_t skipFieldIndex = -1) override;
    virtual void updateStatus(bool forceRedraw = false) override;

    // the return value indicates whether the event should propagate
    virtual bool onTouchEvent(Touch::Pad *pad, Touch::Event event) override;

    virtual uint16_t lockedFg() override;
    virtual uint16_t lockedBg() override;
    virtual uint16_t unlockedFg() override;
    virtual uint16_t unlockedBg() override;

    void backlight(uint8_t state);
    void diag(bool send = true);

   protected:
    Arduino_GFX *device;
};

#endif