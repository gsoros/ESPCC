#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <U8g2lib.h>

SoftwareSerial ss;
TinyGPSPlus gps;
U8G2_SH1106_128X64_VCOMH0_F_HW_I2C oled(
    U8G2_R1,
    U8X8_PIN_NONE,
    GPIO_NUM_18,
    GPIO_NUM_5);

void setup() {
    Serial.begin(115200);
    Serial.println("--------");
    ss.begin(9600, SWSERIAL_8N1, GPIO_NUM_17, GPIO_NUM_16);
    oled.begin();
    oled.setFont(u8g2_font_logisoso32_tn);
}

void loop() {
    while (ss.available() > 0) {
        char c = ss.read();
        gps.encode(c);
        // Serial.print(c);
    }
    if (gps.location.isValid()) {
        Serial.printf(
            "%f %f %.1fm %.2fkm/h %d %d%ssat %02d:%02d:%02d%s\n",  //
            gps.location.lat(),
            gps.location.lng(),
            gps.altitude.meters(),
            gps.speed.kmph(),
            gps.hdop.value(),
            gps.satellites.value(),
            gps.satellites.isValid() ? "" : "X",
            gps.time.hour(),
            gps.time.minute(),
            gps.time.second(),
            gps.time.isValid() ? "" : "X");

        oled.clearBuffer();
        oled.setCursor(0, 32);
        oled.printf("%02d%d",
                    gps.time.hour(),
                    gps.time.minute() / 10);
        oled.setCursor(0, 82);
        oled.printf("%d%02d",
                    gps.time.minute() % 10,
                    gps.time.second());
        oled.setCursor(0, 128);
        oled.printf("%d%02d",
                    gps.date.month() % 10,
                    gps.date.day());
        oled.sendBuffer();
        return;
    }
    static uint32_t satellites = UINT32_MAX;
    if (satellites != gps.satellites.value()) {
        satellites = gps.satellites.value();
        oled.clearBuffer();
        oled.setCursor(0, 82);
        oled.printf("%03d", satellites);
        oled.sendBuffer();
    }
}