#ifndef __display_h
#define __display_h

#include <CircularBuffer.h>

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
        FC_BATTERY_HEARTRATE,
        FC_BATTERY_VESC,
        FC_RANGE
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

    struct Field {
        Area area;
        FieldContent content[DISPLAY_NUM_PAGES];
        bool enabled;
        uint8_t id;
        uint8_t *font;
        uint8_t *labelFont;
        uint8_t *smallFont;
        uint8_t smallFontWidth;

        Field(uint8_t id = 0);
        void setEnabled(bool state = true);
    };

    typedef std::function<void()> QueueItemCallback;
    // typedef void (*QueueItemCallback)();

    struct QueueItem {
        ulong after = 0;
        QueueItemCallback callback = nullptr;
        char tag[32] = "";
    };

    const char *taskName() { return "Display"; }

    Display(uint16_t width,
            uint16_t height,
            uint16_t feedbackWidth = 3,
            uint16_t fieldHeight = 32,
            SemaphoreHandle_t *mutex = nullptr);
    virtual ~Display();

    virtual void setup();
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
    virtual void clock(bool send = true, bool clear = false, int8_t skipFieldIndex = -1) = 0;

    virtual void setClip(const Area *a);
    virtual void setMaxClip();
    virtual void setColor(uint16_t color);
    virtual uint16_t getColor();
    virtual void setColor(uint16_t fg, uint16_t bg);
    virtual void setBgColor(uint16_t color);
    virtual size_t print(const char *str);

    virtual size_t printUnrestricted(const char *str);
    virtual void printField(const uint8_t fieldIndex,
                            const char *str,
                            const bool send = true,
                            const uint8_t *font = nullptr);
    virtual void printfFieldDigits(const uint8_t fieldIndex,
                                   const bool send,
                                   const char *format,
                                   ...);
    virtual void printfFieldChars(const uint8_t fieldIndex,
                                  const bool send,
                                  const char *format,
                                  ...);
    virtual void printFieldDouble(const uint8_t fieldIndex,
                                  double value,
                                  const bool send = true);
    virtual void printField2plus1(const uint8_t fieldIndex,
                                  const char *two,
                                  const char *one,
                                  const bool send = true);
    // get index of field for content or -1
    virtual int8_t getFieldIndex(FieldContent content);
    virtual void displayFieldValues(bool send = true);
    virtual void displayFieldContent(uint8_t fieldIndex,
                                     FieldContent content,
                                     bool send = true);
    virtual int fieldLabel(FieldContent content, char *buf, size_t len);
    virtual uint8_t fieldLabelVPos(uint8_t fieldHeight);
    virtual void splash(bool send = true);
    virtual void message(const char *m, uint8_t fieldIndex = 0, bool send = true);

    // direction <0: down, 0: display labels only, 0<: up
    virtual void switchPage(int8_t direction = 1);

    virtual bool validFieldIndex(uint8_t fieldIndex);
    virtual void setEnabled(uint8_t fieldIndex, bool state = true);
    virtual bool enabled(uint8_t fieldIndex);

    virtual bool setContrast(uint8_t percent);
    virtual void onPower(int16_t value);
    virtual void displayPower(int8_t fieldIndex = -1, bool send = true);
    virtual void onWeight(double value);
    virtual void displayWeight(int8_t fieldIndex = -1, bool send = true);
    virtual void onCadence(int16_t value);
    virtual void displayCadence(int8_t fieldIndex = -1, bool send = true);
    virtual void onHeartrate(int16_t value);
    virtual void displayHeartrate(int8_t fieldIndex = -1, bool send = true);
    virtual void onSpeed(double value);
    virtual void displaySpeed(int8_t fieldIndex = -1, bool send = true);
    virtual void onDistance(uint value);
    virtual void displayDistance(int8_t fieldIndex = -1, bool send = true);
    virtual void onAltGain(uint16_t value);
    virtual void displayAltGain(int8_t fieldIndex = -1, bool send = true);
    virtual void onBattery(int8_t value);
    virtual void printBattCharging(int8_t fieldIndex = -1, bool send = true);
    virtual void displayBattery(int8_t fieldIndex = -1, bool send = true);
    virtual void onBattPM(int8_t value);
    virtual void displayBattPM(int8_t fieldIndex = -1, bool send = true);
    virtual void onBattHRM(int8_t value);
    virtual void displayBattHRM(int8_t fieldIndex = -1, bool send = true);
    virtual void onBattVesc(int8_t value);
    virtual void displayBattVesc(int8_t fieldIndex = -1, bool send = true);
    virtual void onRange(int16_t value);
    virtual void displayRange(int8_t fieldIndex = -1, bool send = true);
    virtual void onPMDisconnected();
    virtual void onHRMDisconnected();
    virtual void onVescConnected();
    virtual void onVescDisconnected();
    virtual void onOta(const char *str);
    virtual void onTare();
    // the return value indicates whether the event should propagate
    virtual bool onTouchEvent(Touch::Pad *pad, Touch::Event event);
    virtual void updateStatus(bool forceRedraw = false);
    virtual void onWifiStateChange();
    virtual void onPasChange();
    virtual void logArea(Area *a, Area *b, const char *str, const char *areaType) const;
    virtual void logAreas(Area *a, const char *str) const;
    // returns false if queue is full or item cannot be inserted
    virtual bool queue(QueueItemCallback callback, uint16_t delayMs, const char *tag = nullptr);
    // returns false if queue is full or item cannot be inserted
    virtual bool queue(QueueItem item);
    virtual int unqueue(const char *tag = nullptr);
    virtual void taskStart(float freq = -1,
                           uint32_t stack = 0,
                           int8_t priority = -1,
                           int8_t core = -1) override;
    virtual void onLockChanged(bool locked);
    virtual void lockedFeedback(uint8_t padIndex, uint16_t color, uint16_t delayMs = 300);
    virtual uint16_t lockedFg();
    virtual uint16_t lockedBg();
    virtual uint16_t unlockedFg();
    virtual uint16_t unlockedBg();
    virtual uint16_t tareFg();
    virtual uint16_t tareBg();
    virtual uint16_t pasFg();
    virtual uint16_t pasBg();

    static uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b) {
        return (((r & 0xf8) << 8) + ((g & 0xfc) << 3) + (b >> 3));
    }

   protected:
    uint16_t width = 0;   // display width
    uint16_t height = 0;  // display height

    uint16_t fg = WHITE;  // 16-bit RGB (5-6-5) foreground color
    uint16_t bg = BLACK;  // 16-bit RGB (5-6-5) background color

    uint8_t *fieldFont = nullptr;  // font for displaying field content
    uint8_t *smallFont = nullptr;  // small characters for field content
    uint8_t *timeFont = nullptr;   // font for displaying the time
    uint8_t timeFontHeight = 0;    //
    uint8_t *dateFont = nullptr;   // font for displaying the date
    uint8_t dateFontHeight = 0;    //
    uint8_t *labelFont = nullptr;  // font for displaying the field labels
    uint8_t labelFontHeight = 0;   //

    Field field[DISPLAY_NUM_FIELDS];               // output fields
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
    double speed = 0.0;
    uint distance = 0;
    uint16_t altGain = 0;
    int8_t battery = -1;
    int8_t battPM = -1;
    int8_t battHRM = -1;
    int8_t battVesc = -1;
    int16_t range = -1;

    enum motionStates {
        msUnknown,
        msStanding,
        msWalking,
        msCycling
    };
    uint8_t motionState = motionStates::msUnknown;

    enum wifiStates {
        wsUnknown,
        wsDisabled,
        wsEnabled,
        wsConnected
    };
    uint8_t wifiState = wsUnknown;

    float defaultTaskFreq = 0;
    TickType_t defaultTaskDelay = 0;
    QueueHandle_t _queue;
    SemaphoreHandle_t defaultMutex;
    SemaphoreHandle_t *mutex = &defaultMutex;

    virtual bool aquireMutex(uint32_t timeout = 100);
    virtual void releaseMutex();
};

#endif