#ifndef __oled_h
#define __oled_h

#include "touch.h"  // avoid "NUM_PADS redefined"
#include "atoll_oled.h"

#define OLED_NUM_PAGES 3
#define OLED_NUM_FIELDS 3
#define OLED_NUM_FEEDBACK 4

class Oled : public Atoll::Oled {
   public:
    enum FieldContent {
        FC_EMPTY,
        FC_POWER,
        FC_CADENCE,
        FC_HEARTRATE,
        FC_SPEED,
        FC_DISTANCE,
        FC_ALTGAIN,
        FC_MOVETIME,
        FC_LAP_TIME,
        FC_LAP_DISTANCE,
        FC_LAP_POWER,
        FC_SATELLITES,
        FC_BATTERY,
        FC_BATTERY_POWER,
        FC_BATTERY_CADENCE,
        FC_BATTERY_HEARTRATE
    };

    struct Area {
        uint8_t x;  // top left x
        uint8_t y;  // top left y
        uint8_t w;  // width
        uint8_t h;  // height
        // bool invert = false;  // unused
    };

    struct OutputField {
        Area area;
        FieldContent content[OLED_NUM_PAGES];
    };

    uint8_t feedbackWidth;                                    // touch feedback area width
    uint8_t fieldWidth;                                       // width of the number output fields
    uint8_t fieldHeight;                                      // height of the number output fields
    uint8_t fieldVSeparation;                                 // vertical distance between the number output fields
    OutputField field[OLED_NUM_FIELDS];                       // output fields
    const uint8_t numFields = OLED_NUM_FIELDS;                // helper
    Area feedback[OLED_NUM_FEEDBACK];                         // touch feedback areas
    const uint8_t *fieldDigitFont = u8g2_font_logisoso32_tn;  // font for displaying up to 3 digits
    const uint8_t *smallDigitFont = u8g2_font_logisoso18_tn;  // font for displaying small digits
    const uint8_t *fieldFont = u8g2_font_logisoso32_tr;       // font for displaying field chars
    const uint8_t *smallFont = u8g2_font_logisoso18_tr;       // font for displaying small chars
    const uint8_t *timeFont = u8g2_font_logisoso28_tn;        // font for displaying the time
    const uint8_t timeFontHeight = 28;                        //
    const uint8_t *dateFont = u8g2_font_fur14_tn;             // font for displaying the date
    const uint8_t dateFontHeight = 14;                        //
    const uint8_t *labelFont = u8g2_font_bytesize_tr;         // font for displaying the field labels
    const uint8_t labelFontHeight = 12;                       //
    Area timeArea;                                            //
    int8_t lastMinute = -1;                                   // last minute displayed
    uint8_t currentPage = 0;                                  //
    const uint8_t numPages = OLED_NUM_PAGES;                  // helper

    Oled(U8G2 *device,
         uint8_t width = 64,
         uint8_t height = 128,
         uint8_t feedbackWidth = 3,
         uint8_t fieldHeight = 32)
        : Atoll::Oled(
              device,
              width,
              height) {
        this->feedbackWidth = feedbackWidth;
        this->fieldHeight = fieldHeight;

        fieldWidth = width - 2 * feedbackWidth;
        fieldVSeparation = (height - 3 * fieldHeight) / 2;

        field[0].area.x = feedbackWidth;
        field[0].area.y = 0;
        field[0].area.w = fieldWidth;
        field[0].area.h = fieldHeight;

        field[1].area.x = feedbackWidth;
        field[1].area.y = fieldHeight + fieldVSeparation;
        field[1].area.w = fieldWidth;
        field[1].area.h = fieldHeight;

        field[2].area.x = feedbackWidth;
        field[2].area.y = height - fieldHeight;
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

        timeArea.x = field[0].area.x;
        timeArea.y = field[0].area.y;
        timeArea.w = field[0].area.w;
        timeArea.h = field[0].area.h + fieldVSeparation + field[1].area.h;
    }

    void setup();
    void loop();

    void printField(const uint8_t fieldIndex,
                    const char *text,
                    const bool send = true,
                    const uint8_t *font = nullptr,
                    const uint8_t color = C_FG,
                    const uint8_t bgColor = C_BG) {
        assert(fieldIndex < sizeof(field) / sizeof(field[0]));
        if (send && !aquireMutex()) return;
        Area *a = &field[fieldIndex].area;
        device->setClipWindow(a->x, a->y, a->x + a->w, a->y + a->h);
        fill(a, bgColor, false);
        device->setCursor(a->x, a->y + a->h);
        device->setDrawColor(color);
        device->setFont(nullptr != font ? font : fieldDigitFont);
        device->print(text);
        device->setMaxClipWindow();
        if (!send) return;
        device->sendBuffer();
        releaseMutex();
    }

    void printfFieldDigits(const uint8_t fieldIndex,
                           const bool send,
                           const uint8_t color,
                           const uint8_t bgColor,
                           const char *format,
                           ...) {
        char out[4];
        va_list argp;
        va_start(argp, format);
        vsnprintf(out, 4, format, argp);
        va_end(argp);
        if (send && !aquireMutex()) return;
        if ((0 == fieldIndex || 1 == fieldIndex) && 0 < lastMinute)
            displayClock(false, true, !fieldIndex);  // clear clock area, update other field
        // log_i("printing '%s' to field %d", out, fieldIndex);
        printField(fieldIndex, out, false, fieldDigitFont, color, bgColor);
        if (send) {
            device->sendBuffer();
            releaseMutex();
        }
        if (0 == fieldIndex || 1 == fieldIndex)
            lastMinute = -1;  // trigger time display
    }

    void printfFieldChars(const uint8_t fieldIndex,
                          const bool send,
                          const uint8_t color,
                          const uint8_t bgColor,
                          const char *format,
                          ...) {
        char out[5];
        va_list argp;
        va_start(argp, format);
        vsnprintf(out, 5, format, argp);
        va_end(argp);
        if (send && !aquireMutex()) return;
        if ((0 == fieldIndex || 1 == fieldIndex) && 0 < lastMinute)
            displayClock(false, true, !fieldIndex);  // clear clock area, update other field
        // log_i("printing '%s' to field %d", out, fieldIndex);
        printField(fieldIndex, out, false, fieldFont, color, bgColor);
        if (send) {
            device->sendBuffer();
            releaseMutex();
        }
        if (0 == fieldIndex || 1 == fieldIndex)
            lastMinute = -1;  // trigger time display
    }

    void printFieldDouble(const uint8_t fieldIndex,
                          double value,
                          const bool send = true,
                          const uint8_t color = C_FG,
                          const uint8_t bgColor = C_BG) {
        while (100.0 <= value) value /= 10.0;  // reduce to 2 digits before the decimal point
        char dec[3];
        snprintf(dec, sizeof(dec), "%2d", (int)value);  // digits before the decimal point
        char rem[2];
        snprintf(rem, sizeof(rem), "%1d", ((int)(value * 10)) % 10);  // first digit after the decimal point
        printField2plus1(fieldIndex, dec, rem, send, color, bgColor);
    }

    void printField2plus1(const uint8_t fieldIndex,
                          const char *two,
                          const char *one,
                          const bool send = true,
                          const uint8_t color = C_FG,
                          const uint8_t bgColor = C_BG) {
        Area *a = &field[fieldIndex].area;
        if (send && !aquireMutex()) return;
        if ((0 == fieldIndex || 1 == fieldIndex) && 0 < lastMinute)
            displayClock(false, true, !fieldIndex);  // clear clock area, update other field
        device->setClipWindow(a->x, a->y, a->x + a->w, a->y + a->h);
        fill(a, bgColor, false);
        device->setDrawColor(color);
        device->setFont(fieldFont);
        device->setCursor(a->x, a->y + a->h);
        device->print(two);
        device->setFont(smallFont);
        device->setCursor(a->x + a->w - 13, a->y + a->h);
        device->print(one);
        device->setMaxClipWindow();
        if (!send) return;
        device->sendBuffer();
        releaseMutex();
    }

    void fill(const Area *a,
              const uint8_t color = C_BG,
              const bool send = true,
              const uint32_t timeout = 100) {
        if (send && !aquireMutex(timeout)) return;
        device->setDrawColor(color);
        device->drawBox(a->x, a->y, a->w, a->h);
        if (!send) return;
        device->sendBuffer();
        releaseMutex();
    }

    void displayClock(bool send = true, bool clear = false, int8_t redrawFieldIndex = -1) {
        static const Area *a = &timeArea;
        if (-2 == lastMinute) return;  // avoid recursion
        tm t = Atoll::localTm();
        if (t.tm_min == lastMinute && !clear) return;
        if (send && !aquireMutex()) return;
        device->setClipWindow(a->x, a->y, a->x + a->w, a->y + a->h);
        fill(a, C_BG, false);
        if (clear) {
            device->setMaxClipWindow();
            lastMinute = -2;  // avoid recursion
            if (-1 != redrawFieldIndex)
                displayFieldContent(
                    redrawFieldIndex,
                    field[redrawFieldIndex].content[currentPage],
                    false);
            if (!send) return;
            device->sendBuffer();
            releaseMutex();
            return;
        }
        device->setCursor(a->x + a->w - timeFontHeight, a->y);
        device->setDrawColor(C_FG);
        device->setFont(timeFont);
        device->setFontDirection(1);  // 90˚
        device->printf("%d:%02d", t.tm_hour, t.tm_min);
        device->setCursor(a->x, a->y);
        device->setFont(dateFont);
        device->printf("%d.%02d", t.tm_mon + 1, t.tm_mday);
        device->setFontDirection(0);  // 0˚
        device->setMaxClipWindow();
        lastMinute = t.tm_min;
        if (!send) return;
        device->sendBuffer();
        releaseMutex();
    }

    // get index of field for content or -1
    int8_t getFieldIndex(FieldContent content) {
        for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
            if (content == field[i].content[currentPage]) return i;
        return -1;
    }

    void displayFieldValues(bool send = true) {
        for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
            displayFieldContent(i, field[i].content[currentPage], send);
    }

    int16_t power = 0;

    void onPower(int16_t value) {
        static int16_t lastPower = 0;
        power = value;
        if (lastPower == power) return;
        displayPower();
        lastPower = power;
    }

    void displayPower(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_POWER);
        if (fieldIndex < 0) return;
        if (0 <= power)
            printfFieldDigits(fieldIndex, send, C_FG, C_BG, "%3d", power);
        else
            printfFieldChars(fieldIndex, send, C_FG, C_BG, " --");
        lastFieldUpdate = millis();
    }

    int16_t cadence = 0;

    void onCadence(int16_t value) {
        static int16_t lastCadence = 0;
        cadence = value;
        if (lastCadence == cadence) return;
        displayCadence();
        lastCadence = cadence;
    }

    void displayCadence(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_CADENCE);
        if (fieldIndex < 0) return;
        if (0 <= cadence)
            printfFieldDigits(fieldIndex, send, C_FG, C_BG, "%3d", cadence);
        else
            printfFieldChars(fieldIndex, send, C_FG, C_BG, " --");
        lastFieldUpdate = millis();
    }

    int16_t heartrate = 0;

    void onHeartrate(int16_t value) {
        static int16_t lastHeartrate = 0;
        heartrate = value;
        if (lastHeartrate == heartrate) return;
        displayHeartrate();
        lastHeartrate = heartrate;
    }

    void displayHeartrate(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_HEARTRATE);
        if (fieldIndex < 0) return;
        if (0 < heartrate)
            printfFieldDigits(fieldIndex, send, C_FG, C_BG, "%3d", heartrate);
        else
            printfFieldChars(fieldIndex, send, C_FG, C_BG, " --");
        lastFieldUpdate = millis();
    }

    double speed = 0;
    int8_t motionState = 0;

    void onSpeed(double value);

    void displaySpeed(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_SPEED);
        if (fieldIndex < 0) return;
        if (100.0 <= speed) {
            while (1000.0 <= speed) speed /= 10.0;
            printfFieldDigits(fieldIndex, send, C_FG, C_BG, "%3d", (int)speed);
        } else
            printFieldDouble(fieldIndex, speed, send);
        // printfFieldChars(fieldIndex, send, C_FG, C_BG, "%.1f", speed);
        lastFieldUpdate = millis();
    }

    uint distance = 0;

    void onDistance(uint value) {
        // log_i("%d", value);
        static uint lastDistance = 0;
        distance = value;
        if (lastDistance == distance) return;
        displayDistance();
        lastDistance = distance;
    }

    void displayDistance(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_DISTANCE);
        if (fieldIndex < 0) return;
        // log_i("%d start", fieldIndex);
        if (distance < 1000)
            printfFieldDigits(fieldIndex, send, C_FG, C_BG, "%3d", distance);  // < 1000 m
        else if (distance < 100000)
            printFieldDouble(fieldIndex, (double)distance / 1000.0, send);  // < 100 km
        else {
            while (1000 <= distance) distance /= 10;
            printfFieldDigits(fieldIndex, send, C_FG, C_BG, "%3d", distance);  // >= 100 km
        }
        // log_i("%d done", fieldIndex);

        lastFieldUpdate = millis();
    }

    uint16_t altGain = 0;

    void onAltGain(uint16_t value) {
        static uint16_t lastAltGain = 0;
        altGain = value;
        if (lastAltGain == altGain) return;
        displayAltGain();
        lastAltGain = altGain;
    }

    void displayAltGain(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_ALTGAIN);
        if (fieldIndex < 0) return;
        if (altGain < 1000)
            printfFieldDigits(fieldIndex, send, C_FG, C_BG,
                              "%3d", altGain);
        else
            printfFieldChars(fieldIndex, send, C_FG, C_BG,
                             "%.1fk", (double)altGain / 1000.0);
        lastFieldUpdate = millis();
    }

    int8_t battery = -1;

    void onBattery(int8_t value) {
        // log_i("%d", value);
        static int8_t lastBattery = -1;
        battery = value;
        if (lastBattery == battery) return;
        displayBattery();
        lastBattery = battery;
    }

    void displayBattery(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_BATTERY);
        if (fieldIndex < 0) return;
        char level[3];
        if (battery < 0)
            snprintf(level, 3, "--");
        else
            snprintf(level, 3, "%2d", battery);
        printField2plus1(fieldIndex, level, "%", send);
        lastFieldUpdate = millis();
    }

    int8_t battPM = -1;

    void onBattPM(int8_t value) {
        log_i("%d", value);
        static int8_t lastBattPM = -1;
        battPM = value;
        if (lastBattPM == battPM) return;
        displayBattPM();
        lastBattPM = battPM;
    }

    void displayBattPM(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_BATTERY_POWER);
        if (fieldIndex < 0) return;
        char level[3];
        if (battPM < 0)
            snprintf(level, 3, "--");
        else
            snprintf(level, 3, "%2d", battPM);
        printField2plus1(fieldIndex, level, "%", send);
        lastFieldUpdate = millis();
    }

    int8_t battHRM = -1;

    void onBattHRM(int8_t value) {
        log_i("%d", value);
        static int8_t lastBattHRM = -1;
        battHRM = value;
        if (lastBattHRM == battHRM) return;
        displayBattHRM();
        lastBattHRM = battHRM;
    }

    void displayBattHRM(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_BATTERY_HEARTRATE);
        if (fieldIndex < 0) return;
        char level[3];
        if (battHRM < 0)
            snprintf(level, 3, "--");
        else
            snprintf(level, 3, "%2d", battHRM);
        printField2plus1(fieldIndex, level, "%", send);
        lastFieldUpdate = millis();
    }

    void onPMDisconnected() {
        onPower(-1);
        onCadence(-1);
        onBattPM(-1);
    }

    void onHRMDisconnected() {
        onHeartrate(-1);
        onBattHRM(-1);
    }

    void displayFieldContent(uint8_t fieldIndex,
                             FieldContent content,
                             bool send = true) {
        switch (content) {
            case FC_EMPTY:
                return;
            case FC_POWER:
                displayPower(fieldIndex, send);
                return;
            case FC_CADENCE:
                displayCadence(fieldIndex, send);
                return;
            case FC_HEARTRATE:
                displayHeartrate(fieldIndex, send);
                return;
            case FC_SPEED:
                displaySpeed(fieldIndex, send);
                return;
            case FC_DISTANCE:
                displayDistance(fieldIndex, send);
                return;
            case FC_ALTGAIN:
                displayAltGain(fieldIndex, send);
                return;
            case FC_MOVETIME:
                log_e("unhandled %d", content);
                return;
            case FC_LAP_TIME:
                log_e("unhandled %d", content);
                return;
            case FC_LAP_DISTANCE:
                log_e("unhandled %d", content);
                return;
            case FC_LAP_POWER:
                log_e("unhandled %d", content);
                return;
            case FC_SATELLITES:
                log_e("unhandled %d", content);
                return;
            case FC_BATTERY:
                displayBattery(fieldIndex, send);
                return;
            case FC_BATTERY_POWER:
            case FC_BATTERY_CADENCE:
                displayBattPM(fieldIndex, send);
                return;
            case FC_BATTERY_HEARTRATE:
                displayBattHRM(fieldIndex, send);
                return;
            default:
                log_e("unhandled %d", content);
        }
    }

    virtual int fieldLabel(FieldContent content, char *buf, size_t len) {
        switch (content) {
            case FC_EMPTY:
                return snprintf(buf, len, "---");
            case FC_POWER:
                return snprintf(buf, len, "Power");
            case FC_CADENCE:
                return snprintf(buf, len, "Cadence");
            case FC_HEARTRATE:
                return snprintf(buf, len, "HR");
            case FC_SPEED:
                return snprintf(buf, len, "Speed");
            case FC_DISTANCE:
                return snprintf(buf, len, "Dist.");
            case FC_ALTGAIN:
                return snprintf(buf, len, "Alt. gain");
            case FC_MOVETIME:
                return snprintf(buf, len, "Move time");
            case FC_LAP_TIME:
                return snprintf(buf, len, "Lap time");
            case FC_LAP_DISTANCE:
                return snprintf(buf, len, "Lap dist.");
            case FC_LAP_POWER:
                return snprintf(buf, len, "Lap power");
            case FC_SATELLITES:
                return snprintf(buf, len, "Sat.");
            case FC_BATTERY:
                return snprintf(buf, len, "Battery");
            case FC_BATTERY_POWER:
            case FC_BATTERY_CADENCE:
                return snprintf(buf, len, "PM Bat.");
            case FC_BATTERY_HEARTRATE:
                return snprintf(buf, len, "HRM Bat.");
            default:
                log_e("unhandled %d", content);
                return -1;
        }
    }

    void displayStatus();
    void onTouchEvent(Touch::Pad *pad, Touch::Event event);

    int8_t wifiState = -1;
    void onWifiEvent();
};

#endif