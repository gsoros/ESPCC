#ifndef __display_h
#define __display_h

#include "definitions.h"

#if DISPLAY_DEVICE == DISPLAY_OLED
#define DISPLAY_NUM_FIELDS DISPLAY_OLED_NUM_FIELDS
#elif DISPLAY_DEVICE == DISPLAY_LCD
#define DISPLAY_NUM_FIELDS DISPLAY_LCD_NUM_FIELDS
#else
#error Could not define DISPLAY_NUM_FIELDS
#endif

#include "atoll_task.h"
#include "atoll_time.h"
#include "atoll_log.h"
#include "touch.h"

// display interface for oled or lcd
class Display : public Atoll::Task, public Print {
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
        int16_t x;  // top left x
        int16_t y;  // top left y
        int16_t w;  // width
        int16_t h;  // height

        Area() : x(0), y(0), w(0), h(0){};
        Area(int16_t x, int16_t y, int16_t w, int16_t h) : x(x), y(y), w(w), h(h){};

        bool contains(const int16_t x, const int16_t y) const {
            return this->x <= x &&
                   x <= this->x + this->w &&
                   this->y <= y &&
                   y <= this->y + this->h;
        }
        bool contains(Area *a) const {
            return contains(a->x, a->y) && contains(a->x + a->w, a->y + a->h);
        }

        bool touches(Area *a) const {
            return x <= a->x + a->w &&
                   a->x <= x + w &&
                   y <= a->y + a->h &&
                   a->y <= y + h;
        }

        bool equals(Area *a) const {
            return x == a->x && y == a->y && w == a->w && h == a->h;
        }

        void clipTo(const Area *a) {
            if (x < a->x) x = a->x;
            if (y < a->y) y = a->y;
            if (a->x + a->w < x + w) w = a->x + a->w - x;
            if (w < 0) w = 0;
            if (a->y + a->h < y + h) h = a->y + a->h - y;
            if (h < 0) h = 0;
        }
    };

    struct OutputField {
        Area area;
        FieldContent content[DISPLAY_NUM_PAGES];
        uint8_t *font;
        uint8_t *labelFont;
        uint8_t *smallFont;
        uint8_t smallFontWidth;

        OutputField() : area() {
            for (uint8_t i = 0; i < DISPLAY_NUM_PAGES; i++)
                content[i] = FC_EMPTY;
            font = nullptr;
            labelFont = nullptr;
            smallFont = nullptr;
            smallFontWidth = 0;
        };
    };

    const char *taskName() { return "Display"; }

    Display(uint16_t width,
            uint16_t height,
            uint16_t feedbackWidth = 3,
            uint16_t fieldHeight = 32,
            SemaphoreHandle_t *mutex = nullptr)
        : width(width),
          height(height),
          feedbackWidth(feedbackWidth),
          fieldHeight(fieldHeight) {
        for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
            field[i] = OutputField();
        for (uint8_t i = 0; i < sizeof(feedback) / sizeof(feedback[0]); i++)
            feedback[i] = Area();

        if (nullptr == mutex)
            defaultMutex = xSemaphoreCreateMutex();
        else
            this->mutex = mutex;
    }

    virtual ~Display() {}

    virtual void setup() {
        // log_i("setMaxClip()");
        setMaxClip();
    }

    virtual void loop();

    virtual size_t write(uint8_t) = 0;
    virtual void fill(const Area *a, uint16_t color, bool send = true) = 0;
    virtual void fillUnrestricted(const Area *a, uint16_t color, bool send = true) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    virtual void sendBuffer() = 0;
    virtual void drawXBitmap(int16_t x,
                             int16_t y,
                             int16_t w,
                             int16_t h,
                             const uint8_t *bitmap,
                             uint16_t color,
                             bool send = true) = 0;
    virtual void setClip(int16_t x, int16_t y, int16_t w, int16_t h) = 0;
    virtual void setFont(const uint8_t *font) = 0;

    virtual void setClip(const Area *a) {
        setClip(a->x, a->y, a->w, a->h);
    }
    virtual void setMaxClip() { setClip(0, 0, width, height); }

    virtual void setColor(int16_t color) { fg = color; }
    virtual void setColor(int16_t fg, int16_t bg) {
        this->fg = fg;
        this->bg = bg;
    }
    virtual void setBgColor(int16_t color) { bg = color; }

    virtual size_t print(const char *str);
    virtual size_t printUnrestricted(const char *str) {
        return Print::print(str);
    }

    virtual void printField(const uint8_t fieldIndex,
                            const char *str,
                            const bool send = true,
                            const uint8_t *font = nullptr) {
        // log_i("field %d %s", fieldIndex, str);
        assert(fieldIndex < sizeof(field) / sizeof(field[0]));
        if (send && !aquireMutex()) return;
        Area *a = &field[fieldIndex].area;
        setClip(a);
        fill(a, bg, false);
        setCursor(a->x, a->y + a->h - 1);
        if (font)
            setFont(font);
        else
            setFont(field[fieldIndex].font);
        print(str);
        setMaxClip();
        if (!send) return;
        sendBuffer();
        releaseMutex();
    }

    virtual void printfFieldDigits(const uint8_t fieldIndex,
                                   const bool send,
                                   const char *format,
                                   ...) {
        if (send && !aquireMutex()) return;
        char out[4];
        va_list argp;
        va_start(argp, format);
        vsnprintf(out, 4, format, argp);
        va_end(argp);

        if (0 < lastMinute)
            clock(false, true, fieldIndex);  // clear clock area, update other fields
        // log_i("printing '%s' to field %d", out, fieldIndex);

        printField(fieldIndex, out, false);
        if (send) {
            sendBuffer();
            releaseMutex();
        }

        // if (0 == fieldIndex || 1 == fieldIndex)
        //     lastMinute = -1;  // trigger time display
    }

    virtual void printfFieldChars(const uint8_t fieldIndex,
                                  const bool send,
                                  const char *format,
                                  ...) {
        if (send && !aquireMutex()) return;
        char out[5];
        va_list argp;
        va_start(argp, format);
        vsnprintf(out, 5, format, argp);
        va_end(argp);

        if (0 < lastMinute)
            clock(false, true, fieldIndex);  // clear clock area, update other fields
        // log_i("printing '%s' to field %d", out, fieldIndex);

        printField(fieldIndex, out, false);
        if (send) {
            sendBuffer();
            releaseMutex();
        }
        // if (0 == fieldIndex || 1 == fieldIndex)
        //     lastMinute = -1;  // trigger time display
    }

    virtual void printFieldDouble(const uint8_t fieldIndex,
                                  double value,
                                  const bool send = true) {
        if (100.0 <= abs(value)) {
            log_w("reducing %.2f", value);
            while (100.0 <= abs(value)) value /= 10.0;  // reduce to 2 digits before the decimal point
        }
        if (value < -10.0) {
            // -10.1 will be printed as "-10"
            printfFieldDigits(fieldIndex, send, "%3d", (int)value);
            return;
        }
        char inte[3];
        snprintf(inte, sizeof(inte), "%2.0f", value);  // digits before the decimal point
        char dec[2];
        snprintf(dec, sizeof(dec), "%1d", abs((int)(value * 10)) % 10);  // first digit after the decimal point
        printField2plus1(fieldIndex, inte, dec, send);
    }

    virtual void printField2plus1(const uint8_t fieldIndex,
                                  const char *two,
                                  const char *one,
                                  const bool send = true) {
        Area *a = &field[fieldIndex].area;
        if (send && !aquireMutex()) return;
        if (0 < lastMinute)
            clock(false, true, fieldIndex);  // clear clock area, update other fields
        setClip(a);
        fill(a, bg, false);
        setFont(field[fieldIndex].font);
        setCursor(a->x, a->y + a->h - 1);
        print(two);
        setFont(field[fieldIndex].smallFont);
        setCursor(a->x + a->w - field[fieldIndex].smallFontWidth, a->y + a->h);
        print(one);
        setMaxClip();
        if (!send) return;
        sendBuffer();
        releaseMutex();
    }

    // get index of field for content or -1
    virtual int8_t getFieldIndex(FieldContent content) {
        for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
            if (content == field[i].content[currentPage]) return i;
        return -1;
    }

    virtual void displayFieldValues(bool send = true) {
        for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
            displayFieldContent(i, field[i].content[currentPage], send);
    }

    virtual void displayFieldContent(uint8_t fieldIndex,
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

    virtual void splash() {
        if (!aquireMutex()) return;
        Area *a = &field[0].area;
        setClip(a);
        setFont(smallFont);
        setCursor(a->x + 10, a->y + a->h - 5);
        print("espcc");
        sendBuffer();
        setMaxClip();
        releaseMutex();
    }

    virtual void clock(bool send = true, bool clear = false, int8_t skipFieldIndex = -1) = 0;

    virtual bool setContrast(uint8_t percent) {
        log_e("501");
        return false;
    }

    virtual void onPower(int16_t value) {
        static int16_t lastPower = 0;
        power = value;
        if (lastPower == power) return;
        if (lastWeightUpdate + 1000 < millis())
            displayPower();
        lastPower = power;
    }

    virtual void displayPower(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_POWER);
        if (fieldIndex < 0) return;
        if (0 <= power)
            printfFieldDigits(fieldIndex, send, "%3d", power);
        else
            fill(&field[fieldIndex].area, bg, send);
        lastFieldUpdate = millis();
        lastPowerUpdate = lastFieldUpdate;
    }

    virtual void onWeight(double value) {
        static double lastWeight = 0.0;
        weight = value;
        if (lastWeight == weight) return;
        if (lastPowerUpdate + 1000 < millis())
            displayWeight();
        lastWeight = weight;
    }

    virtual void displayWeight(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_POWER);
        if (fieldIndex < 0) return;
        printFieldDouble(fieldIndex, weight, send);
        lastFieldUpdate = millis();
        lastWeightUpdate = lastFieldUpdate;
    }

    virtual void onCadence(int16_t value) {
        static int16_t lastCadence = 0;
        cadence = value;
        if (lastCadence == cadence) return;
        displayCadence();
        lastCadence = cadence;
    }

    virtual void displayCadence(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_CADENCE);
        if (fieldIndex < 0) return;
        if (0 <= cadence)
            printfFieldDigits(fieldIndex, send, "%3d", cadence);
        else
            fill(&field[fieldIndex].area, bg, send);
        lastFieldUpdate = millis();
    }

    virtual void onHeartrate(int16_t value) {
        static int16_t lastHeartrate = 0;
        heartrate = value;
        if (lastHeartrate == heartrate) return;
        displayHeartrate();
        lastHeartrate = heartrate;
    }

    virtual void displayHeartrate(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_HEARTRATE);
        if (fieldIndex < 0) return;
        if (0 < heartrate)
            printfFieldDigits(fieldIndex, send, "%3d", heartrate);
        else
            fill(&field[fieldIndex].area, bg, send);
        lastFieldUpdate = millis();
    }

    virtual void onSpeed(double value);

    virtual void displaySpeed(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_SPEED);
        if (fieldIndex < 0) return;
        if (100.0 <= speed) {
            while (1000.0 <= speed) speed /= 10.0;
            printfFieldDigits(fieldIndex, send, "%3d", (int)speed);
        } else
            printFieldDouble(fieldIndex, speed, send);
        lastFieldUpdate = millis();
    }

    virtual void onDistance(uint value) {
        // log_i("%d", value);
        static uint lastDistance = 0;
        distance = value;
        if (lastDistance == distance) return;
        displayDistance();
        lastDistance = distance;
    }

    virtual void displayDistance(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_DISTANCE);
        if (fieldIndex < 0) return;
        if (distance < 1000)
            printfFieldDigits(fieldIndex, send, "%3d", distance);  // < 1000 m
        else if (distance < 100000)
            printFieldDouble(fieldIndex, (double)distance / 1000.0, send);  // < 100 km
        else {
            while (1000 <= distance) distance /= 10;
            printfFieldDigits(fieldIndex, send, "%3d", distance);  // >= 100 km
        }
        lastFieldUpdate = millis();
    }

    virtual void onAltGain(uint16_t value) {
        static uint16_t lastAltGain = 0;
        altGain = value;
        if (lastAltGain == altGain) return;
        displayAltGain();
        lastAltGain = altGain;
    }

    virtual void displayAltGain(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_ALTGAIN);
        if (fieldIndex < 0) return;
        if (altGain < 1000)
            printfFieldDigits(fieldIndex, send, "%3d", altGain);
        else
            printfFieldChars(fieldIndex, send, "%.1fk", (double)altGain / 1000.0);
        lastFieldUpdate = millis();
    }

    virtual void onBattery(int8_t value) {
        // log_i("%d", value);
        static int8_t lastBattery = -1;
        battery = value;
        if (lastBattery == battery) return;
        displayBattery();
        lastBattery = battery;
    }

    virtual void printBattCharging(int8_t fieldIndex = -1,
                                   bool send = true) {
        static const uint8_t chgIconSize = 24;
        static const uint8_t chgIcon[] = {
            0x00, 0x00, 0x10, 0x00, 0x40, 0x18, 0x00, 0xe0, 0x0c, 0x00, 0xb0, 0xc7,
            0x00, 0x18, 0xe7, 0x00, 0x08, 0x76, 0x00, 0x0c, 0x3c, 0x00, 0x0c, 0x18,
            0x00, 0x0c, 0x10, 0x00, 0x08, 0x30, 0x00, 0x1c, 0x18, 0x00, 0x7f, 0x0e,
            0x80, 0xc7, 0x07, 0x80, 0x03, 0x00, 0x80, 0x01, 0x00, 0x80, 0x01, 0x00,
            0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00,
            0x00, 0x03, 0x00, 0xfc, 0x01, 0x00, 0x7e, 0x00, 0x00, 0x03, 0x00, 0x00};
        if (fieldIndex < 0) return;
        Area *a = &field[fieldIndex].area;
        if (send && !aquireMutex()) return;
        setClip(a);
        fill(a, bg, false);
        drawXBitmap(a->x + a->w - chgIconSize, a->y, chgIconSize, chgIconSize, chgIcon, fg, false);
        setMaxClip();
        if (!send) return;
        sendBuffer();
        releaseMutex();
    }

    virtual void displayBattery(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_BATTERY);
        if (fieldIndex < 0) return;
        if (99 <= battery) {
            printBattCharging(fieldIndex, send);
        } else if (0 <= battery) {
            char level[3] = "";
            snprintf(level, 3, "%2d", battery);
            printField2plus1(fieldIndex, level, "%", send);
        } else
            fill(&field[fieldIndex].area, bg, send);
        lastFieldUpdate = millis();
    }

    virtual void onBattPM(int8_t value) {
        // log_i("%d", value);
        static int8_t lastBattPM = -1;
        battPM = value;
        if (lastBattPM == battPM) return;
        displayBattPM();
        lastBattPM = battPM;
    }

    virtual void displayBattPM(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_BATTERY_POWER);
        if (fieldIndex < 0) return;
        if (99 <= battPM) {
            printBattCharging(fieldIndex, send);
        } else if (0 <= battPM) {
            char level[3] = "";
            snprintf(level, 3, "%2d", battPM);
            printField2plus1(fieldIndex, level, "%", send);
        } else
            fill(&field[fieldIndex].area, bg, send);
        lastFieldUpdate = millis();
    }

    virtual void onBattHRM(int8_t value) {
        log_i("%d", value);
        static int8_t lastBattHRM = -1;
        battHRM = value;
        if (lastBattHRM == battHRM) return;
        displayBattHRM();
        lastBattHRM = battHRM;
    }

    virtual void displayBattHRM(int8_t fieldIndex = -1, bool send = true) {
        if (fieldIndex < 0)
            fieldIndex = getFieldIndex(FC_BATTERY_HEARTRATE);
        if (fieldIndex < 0) return;
        if (99 <= battHRM) {
            printBattCharging(fieldIndex, send);
        } else if (0 <= battHRM) {
            char level[3] = "";
            snprintf(level, 3, "%2d", battHRM);
            printField2plus1(fieldIndex, level, "%", send);
        } else
            fill(&field[fieldIndex].area, bg, send);
        lastFieldUpdate = millis();
    }

    virtual void onPMDisconnected() {
        onPower(-1);
        onCadence(-1);
        onBattPM(-1);
    }

    virtual void onHRMDisconnected() {
        onHeartrate(-1);
        onBattHRM(-1);
    }

    virtual void onOta(const char *str) {
        if (!aquireMutex()) return;
        Area a = Area(0, 0, width, height - statusArea.h);
        fillUnrestricted(&a, bg, false);
        setCursor(3, height / 2);
        char out[16];
        snprintf(out, sizeof(out), "OTA: %s", str);
        setFont(labelFont);
        setColor(fg);
        printUnrestricted(out);
        sendBuffer();
        releaseMutex();
    }

    virtual void updateStatus();
    virtual void onTouchEvent(Touch::Pad *pad, Touch::Event event);
    virtual void onWifiStateChange();

    virtual void logArea(Area *a, Area *b, const char *str, const char *areaType) const {
        if (b->equals(a))
            log_i("%s %s", str, areaType);
        else if (b->contains(a))
            log_i("%s contains %s", str, areaType);
        else if (b->touches(a))
            log_i("%s touches %s", str, areaType);
    }
    virtual void logAreas(Area *a, const char *str) const {
        char areaType[10] = "";
        for (uint8_t i = 0; i < numFields; i++) {
            snprintf(areaType, 10, "field%d", i);
            logArea((Area *)&field[i].area, a, str, areaType);
        }
        for (uint8_t i = 0; i < numFeedback; i++) {
            snprintf(areaType, 10, "feedback%d", i);
            logArea((Area *)&feedback[i], a, str, areaType);
        }
        logArea((Area *)&clockArea, a, str, "clock");
        logArea((Area *)&statusArea, a, str, "status");
    }

    static uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b) {
        return (((r & 0xf8) << 8) + ((g & 0xfc) << 3) + (b >> 3));
    }

   protected:
    uint16_t width = 0;   // display width
    uint16_t height = 0;  // display height

    uint16_t fg = 0xffff;  // 16-bit RGB (5-6-5) foreground color
    uint16_t bg = 0x0000;  // 16-bit RGB (5-6-5) background color

    uint8_t *fieldFont = nullptr;  // font for displaying field content
    uint8_t *smallFont = nullptr;  // small characters for field content
    uint8_t *timeFont = nullptr;   // font for displaying the time
    uint8_t timeFontHeight = 0;    //
    uint8_t *dateFont = nullptr;   // font for displaying the date
    uint8_t dateFontHeight = 0;    //
    uint8_t *labelFont = nullptr;  // font for displaying the field labels
    uint8_t labelFontHeight = 0;   //

    OutputField field[DISPLAY_NUM_FIELDS];         // output fields
    uint8_t fieldWidth = 1;                        // width of the output fields
    uint8_t fieldHeight = 1;                       // height of the output fields
    uint8_t fieldVSeparation = 1;                  // vertical distance between the output fields
    const uint8_t numFields = DISPLAY_NUM_FIELDS;  // helper

    Area feedback[DISPLAY_NUM_FEEDBACK];               // touch feedback areas
    uint8_t feedbackWidth = 1;                         // touch feedback area width
    const uint8_t numFeedback = DISPLAY_NUM_FEEDBACK;  // helper

    Area clockArea = Area();      //
    Area statusArea = Area();     //
    uint8_t statusIconSize = 14;  // = 14

    uint8_t currentPage = 0;
    const uint8_t numPages = DISPLAY_NUM_PAGES;  // helper

    ulong lastFieldUpdate = 0;
    int8_t lastMinute = -1;

    int16_t power = -1;
    ulong lastPowerUpdate = 0;
    double weight = 0.0;
    ulong lastWeightUpdate = 0;
    int16_t cadence = -1;
    int16_t heartrate = 0;
    double speed = 0;
    int8_t motionState = 0;
    uint distance = 0;
    uint16_t altGain = 0;
    int8_t battery = -1;
    int8_t battPM = -1;
    int8_t battHRM = -1;
    int8_t wifiState = -1;  // 0: disabled, 1: enabled, 2: connected

    SemaphoreHandle_t defaultMutex;
    SemaphoreHandle_t *mutex = &defaultMutex;

    virtual bool aquireMutex(uint32_t timeout = 100) {
        // log_d("aquireMutex %d", (int)mutex);
        if (xSemaphoreTake(*mutex, (TickType_t)timeout) == pdTRUE)
            return true;
        log_i("Could not aquire mutex");
        return false;
    }

    virtual void releaseMutex() {
        // log_d("releaseMutex %d", (int)mutex);
        xSemaphoreGive(*mutex);
    }
};

#endif