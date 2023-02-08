#include "board.h"
#include "display.h"
#include "atoll_time.h"

Display::Display(uint16_t width,
                 uint16_t height,
                 uint16_t feedbackWidth,
                 uint16_t fieldHeight,
                 SemaphoreHandle_t *mutex)
    : width(width),
      height(height),
      feedbackWidth(feedbackWidth),
      fieldHeight(fieldHeight) {
    for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
        field[i] = OutputField();
    for (uint8_t i = 0; i < sizeof(feedback) / sizeof(feedback[0]); i++)
        feedback[i] = Area();
    //_queue.clear();
    if (nullptr == mutex)
        defaultMutex = xSemaphoreCreateMutex();
    else
        this->mutex = mutex;
}

Display::~Display() {}

void Display::setup() {
    _queue = xQueueCreate(8, sizeof(QueueItem));
    if (NULL == _queue) log_e("error creating _queue");
    // log_i("setMaxClip()");
    setMaxClip();
}

void Display::loop() {
    ulong t = millis();

    // static ulong lastLoopTime = t;
    // log_i("loop %dms", lastLoopTime ? t - lastLoopTime : 0);
    // lastLoopTime = t;

    uint8_t queueNumItems = (uint8_t)uxQueueMessagesWaiting(_queue);
    if (0 != queueNumItems) {
        ulong nextCallTime = 0;
        for (uint8_t i = 0; i < queueNumItems; i++) {
            QueueItem item;
            if (pdPASS != xQueueReceive(_queue, &(item), 0)) {
                log_e("failed to get item from queue");
                continue;
            }
            if (item.after <= millis()) {
                // log_i("executing queued item #%d", i);
                item.callback();
            } else {
                // log_i("delaying queued item #%d", i);
                queue(item);
                if (!nextCallTime || item.after < nextCallTime)
                    nextCallTime = item.after;
            }
        }
        if (0 == uxQueueMessagesWaiting(_queue)) {
            log_i("queue empty, setting default delay %dms", defaultTaskDelay);
            taskSetDelay(defaultTaskDelay);
        } else {
            // log_i("queue not empty");
            nextCallTime = min(nextCallTime, millis() + pdTICKS_TO_MS(defaultTaskDelay));
            ulong nextWakeTime = taskGetNextWakeTimeMs();
            if (nextCallTime < nextWakeTime) {
                uint16_t delay = 0;
                t = millis();
                if (t < nextCallTime) delay = nextCallTime - t;
                log_i("setting delay %dms", delay);
                taskSetDelay(delay);
            }
        }
    }
    // log_i("queue run done");

    if (!enabled) {
        // log_i("not enabled");
        return;
    }
    static ulong lastStatusUpdate = 0;
    t = millis();
    if (t - 1000 < lastStatusUpdate) return;
    lastStatusUpdate = t;
    // log_i("updateStatus()");
    updateStatus();

    if (lastFieldUpdate < t - 15000 && Atoll::systemTimeLastSet()) {
        // log_i("clock()");
        clock();
    }

    // static ulong lastFake = 0;
    // static double fake = 0.0;
    // if (lastFake + 500 < millis()) {
    //     fake += (double)random(0, 5000) / 10.0;
    //     onDistance((uint)fake);
    //     lastFake = millis();
    // }
}

void Display::setClip(const Area *a) {
    setClip(a->x, a->y, a->w, a->h);
}

void Display::setMaxClip() {
    setClip(0, 0, width, height);
}

void Display::setColor(uint16_t color) {
    fg = color;
}

uint16_t Display::getColor() {
    return fg;
}

void Display::setColor(uint16_t fg, uint16_t bg) {
    this->fg = fg;
    this->bg = bg;
}

void Display::setBgColor(uint16_t color) {
    bg = color;
}

size_t Display::print(const char *str) {
    if (board.otaMode) return 0;
    return printUnrestricted(str);
}

size_t Display::printUnrestricted(const char *str) {
    return Print::print(str);
}

void Display::printField(const uint8_t fieldIndex,
                         const char *str,
                         const bool send,
                         const uint8_t *font) {
    if (!enabled) {
        log_i("not enabled");
        return;
    }
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

void Display::printfFieldDigits(const uint8_t fieldIndex,
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

void Display::printfFieldChars(const uint8_t fieldIndex,
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

void Display::printFieldDouble(const uint8_t fieldIndex,
                               double value,
                               const bool send) {
    if (100.0 <= abs(value)) {
        log_w("reducing %.2f", value);
        while (100.0 <= abs(value)) value /= 10.0;  // reduce to 2 digits before the decimal point
    }
    if (value < -10.0) {
        // -10.1 will be printed as "-10"
        printfFieldDigits(fieldIndex, send, "%3d", (int)value);
        return;
    }
    char n[5];
    snprintf(n, sizeof(n), "%4.1f", value);
    char inte[3];
    inte[0] = n[0];
    inte[1] = n[1];
    inte[2] = 0;
    char dec[2];
    dec[0] = n[3];
    dec[1] = 0;
    printField2plus1(fieldIndex, inte, dec, send);
    // log_i("'%4.1f' -> '%s' -> '%s.%s'", value, n, inte, dec);
}

void Display::printField2plus1(const uint8_t fieldIndex,
                               const char *two,
                               const char *one,
                               const bool send) {
    if (!enabled) {
        log_i("not enabled");
        return;
    }
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
int8_t Display::getFieldIndex(FieldContent content) {
    for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
        if (content == field[i].content[currentPage]) return i;
    return -1;
}

void Display::displayFieldValues(bool send) {
    for (uint8_t i = 0; i < sizeof(field) / sizeof(field[0]); i++)
        displayFieldContent(i, field[i].content[currentPage], send);
}

void Display::displayFieldContent(uint8_t fieldIndex,
                                  FieldContent content,
                                  bool send) {
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
        case FC_BATTERY_VESC:
            displayBattVesc(fieldIndex, send);
            return;
        case FC_RANGE:
            displayRange(fieldIndex, send);
            return;
        default:
            log_e("unhandled %d", content);
    }
}

int Display::fieldLabel(FieldContent content, char *buf, size_t len) {
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
        case FC_BATTERY_VESC:
            return snprintf(buf, len, "Vesc Bat.");
        case FC_RANGE:
            return snprintf(buf, len, "Range");
        default:
            log_e("unhandled %d", content);
            return -1;
    }
}

uint8_t Display::fieldLabelVPos(uint8_t fieldHeight) {
    return fieldHeight / 2;
}

void Display::splash(bool send) {
    message("espcc", send);
}

void Display::message(const char *m, bool send) {
    if (send && !aquireMutex()) return;
    Area *a = &field[0].area;
    setClip(a);
    setFont(smallFont);
    setCursor(a->x + 10, a->y + a->h - 5);
    print(m);
    setMaxClip();
    if (send) {
        sendBuffer();
        releaseMutex();
    }
}

// direction <0: down, >=0: up
void Display::switchPage(int8_t direction) {
    currentPage += 0 <= direction ? 1 : -1;
    if (currentPage < 0) currentPage = numPages - 1;
    if (numPages <= currentPage) currentPage = 0;
    log_i("currentPage %d", currentPage);
    if (!aquireMutex()) return;
    clock(false, true);  // clear clock
    char label[10] = "";
    Area *b;
    for (uint8_t i = 0; i < numFields; i++) {
        if (fieldLabel(field[i].content[currentPage], label, sizeof(label)) < 1)
            continue;
        b = &field[i].area;
        setClip(b);
        fill(b, bg, false);
        setCursor(b->x, b->y + fieldLabelVPos(b->h));
        setFont(field[i].labelFont);
        print(label);
    }
    sendBuffer();
    setMaxClip();
    releaseMutex();
    lastFieldUpdate = millis();
    enabled = false;
    if (queue([this]() {
            enabled = true;
            displayFieldValues();
        },
              3000)) {
        // log_i("queued enable; displayFieldValues() in 3000ms");
    } else {
        enabled = true;
    }
}

bool Display::setContrast(uint8_t percent) {
    log_e("501");
    return false;
}

void Display::onPower(int16_t value) {
    // log_d("value: %d", value);
    static int16_t last = 0;
    power = value;
    if (last == power) return;
    if (lastWeightUpdate + 1000 < millis() || -1 == power)
        displayPower();
    last = power;
}

void Display::displayPower(int8_t fieldIndex, bool send) {
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

void Display::onWeight(double value) {
    static double last = 0.0;
    weight = value;
    if (last == weight) return;
    if (lastPowerUpdate + 1000 < millis())
        displayWeight();
    last = weight;
}

void Display::displayWeight(int8_t fieldIndex, bool send) {
    if (fieldIndex < 0)
        fieldIndex = getFieldIndex(FC_POWER);
    if (fieldIndex < 0) return;
    printFieldDouble(fieldIndex, weight, send);
    lastFieldUpdate = millis();
    lastWeightUpdate = lastFieldUpdate;
}

void Display::onCadence(int16_t value) {
    static int16_t last = 0;
    cadence = value;
    if (last == cadence) return;
    displayCadence();
    last = cadence;
}

void Display::displayCadence(int8_t fieldIndex, bool send) {
    if (fieldIndex < 0)
        fieldIndex = getFieldIndex(FC_CADENCE);
    if (fieldIndex < 0) return;
    if (0 <= cadence)
        printfFieldDigits(fieldIndex, send, "%3d", cadence);
    else
        fill(&field[fieldIndex].area, bg, send);
    lastFieldUpdate = millis();
}

void Display::onHeartrate(int16_t value) {
    static int16_t last = 0;
    heartrate = value;
    if (last == heartrate) return;
    displayHeartrate();
    last = heartrate;
}

void Display::displayHeartrate(int8_t fieldIndex, bool send) {
    if (fieldIndex < 0)
        fieldIndex = getFieldIndex(FC_HEARTRATE);
    if (fieldIndex < 0) return;
    if (0 < heartrate)
        printfFieldDigits(fieldIndex, send, "%3d", heartrate);
    else
        fill(&field[fieldIndex].area, bg, send);
    lastFieldUpdate = millis();
}

void Display::onSpeed(double value) {
    // log_i("%.2f", value);
    static double last = 0;
    speed = value;
    if (last == speed) return;
    displaySpeed();
    last = speed;
    motionState = board.gps.minMovingSpeed <= speed
                      ? 10.0 < speed
                            ? 2
                            : 1
                      : 0;
}

void Display::displaySpeed(int8_t fieldIndex, bool send) {
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

void Display::onDistance(uint value) {
    // log_i("%d", value);
    static uint last = 0;
    distance = value;
    if (last == distance) return;
    displayDistance();
    last = distance;
}

void Display::displayDistance(int8_t fieldIndex, bool send) {
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

void Display::onAltGain(uint16_t value) {
    static uint16_t last = 0;
    altGain = value;
    if (last == altGain) return;
    displayAltGain();
    last = altGain;
}

void Display::displayAltGain(int8_t fieldIndex, bool send) {
    if (fieldIndex < 0)
        fieldIndex = getFieldIndex(FC_ALTGAIN);
    if (fieldIndex < 0) return;
    if (altGain < 1000)
        printfFieldDigits(fieldIndex, send, "%3d", altGain);
    else
        printfFieldChars(fieldIndex, send, "%.1fk", (double)altGain / 1000.0);
    lastFieldUpdate = millis();
}

void Display::onBattery(int8_t value) {
    // log_i("%d", value);
    static int8_t last = -1;
    battery = value;
    if (last == battery) return;
    displayBattery();
    last = battery;
}

void Display::printBattCharging(int8_t fieldIndex, bool send) {
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

void Display::displayBattery(int8_t fieldIndex, bool send) {
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

void Display::onBattPM(int8_t value) {
    // log_i("%d", value);
    static int8_t last = -1;
    battPM = value;
    if (last == battPM) return;
    displayBattPM();
    last = battPM;
}

void Display::displayBattPM(int8_t fieldIndex, bool send) {
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

void Display::onBattHRM(int8_t value) {
    log_i("%d", value);
    static int8_t last = -1;
    battHRM = value;
    if (last == battHRM) return;
    displayBattHRM();
    last = battHRM;
}

void Display::displayBattHRM(int8_t fieldIndex, bool send) {
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

void Display::onBattVesc(int8_t value) {
    log_i("%d", value);
    static int8_t last = -1;
    battVesc = value;
    if (last == battVesc) return;
    displayBattVesc();
    last = battVesc;
}

void Display::displayBattVesc(int8_t fieldIndex, bool send) {
    if (fieldIndex < 0)
        fieldIndex = getFieldIndex(FC_BATTERY_VESC);
    if (fieldIndex < 0) return;
    if (battVesc < 0) {
        fill(&field[fieldIndex].area, bg, send);
        return;
    }
    char level[3] = "";
    snprintf(level, 3, "%2d", 99 < battVesc ? 99 : battVesc);
    printField2plus1(fieldIndex, level, "%", send);
    lastFieldUpdate = millis();
}

void Display::onRange(int16_t value) {
    log_d("value=%d", value);
    // #if (4 <= ATOLL_LOG_LEVEL)  // debug
    //     value /= 10;
    // #endif
    static int16_t last = -1;
    if (-1 == value || -1 == last || INT16_MAX == value || INT16_MAX == last) {
        range = value;
    } else {
        int32_t tmp = (last + value) / 2;
        if (INT16_MAX < tmp) tmp = INT16_MAX;
        range = (int16_t)tmp;
    }
    log_d("last=%d range=%d", last, range);
    if (last == range) return;
    displayRange();
    last = range;
}

void Display::displayRange(int8_t fieldIndex, bool send) {
    if (fieldIndex < 0)
        fieldIndex = getFieldIndex(FC_RANGE);
    if (fieldIndex < 0) return;
    if (range < 0) {
        fill(&field[fieldIndex].area, bg, send);
        return;
    }
    char km[4] = "";
    if (999 < range)
        snprintf(km, 4, "inf");
    else
        snprintf(km, 4, "%3d", range);
    printfFieldDigits(fieldIndex, send, km);
    lastFieldUpdate = millis();
}

void Display::onPMDisconnected() {
    log_d("onPMDisconnected");
    onPower(-1);
    onCadence(-1);
    onBattPM(-1);
}

void Display::onHRMDisconnected() {
    onHeartrate(-1);
    onBattHRM(-1);
}

void Display::onVescConnected() {
    onPasChange();
}

void Display::onVescDisconnected() {
    onRange(-1);
    onBattVesc(-1);
}

void Display::onOta(const char *str) {
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

void Display::onTare() {
    log_i("displaying tare, disabling...");
    fill(&field[0].area, tareBg());
    uint16_t savedFg = getColor();
    setColor(tareFg());
    message("TARE");
    setColor(savedFg);
    enabled = false;
    if (!queue([this]() {
            log_i("...enabling");
            enabled = true;
            displayFieldContent(0, field[0].content[currentPage]);
        },
               3000)) {
        log_w("could not queue displayFieldContent, enabling");
        enabled = true;
    }
}

bool Display::onTouchEvent(Touch::Pad *pad, Touch::Event event) {
    if (nullptr == pad) return false;

    if (board.touch.locked) {
        if (event == Touch::Event::start) {
            lockedFeedback(pad->index, lockedFg());
            return false;
        }
        if (event == Touch::Event::quintupleTouch) {
            return true;  // propagate to unlock
        }
        return false;
    }

    assert(pad->index < sizeof(feedback) / sizeof(feedback[0]));
    Area *a = &feedback[pad->index];
    static const uint16_t feedbackDelay = 600;

    switch (event) {
        case Touch::Event::start: {
            return true;
        }

        case Touch::Event::end: {
            return true;
        }

        case Touch::Event::singleTouch: {
            log_i("pad %d single", pad->index);
            fill(a, fg);
            if (queue([this, a]() { fill(a, bg); }, 300)) {
                // log_i("queued pad #%d clear in 300ms", pad->index);
            }
            if (1 == pad->index) {  // top right to switch page up
                switchPage(1);
                return false;              // do not propagate
            } else if (3 == pad->index) {  // bottom right to switch page down
                switchPage(-1);
                return false;  // do not propagate
            }
            return true;
        }

        case Touch::Event::doubleTouch: {
            log_i("pad %d double", pad->index);
            Area b;
            memcpy(&b, a, sizeof(b));
            b.h /= 3;  // divide area height by 3
            if (aquireMutex()) {
                fill(&b, fg, false);  // area 1 white
                b.y += b.h;           // move down
                fill(&b, bg, false);  // area 2 black
                b.y += b.h;           // move down
                fill(&b, fg, false);  // area 3 white
                sendBuffer();
                releaseMutex();
                if (queue([this, a]() { fill(a, bg, true); }, feedbackDelay)) {
                    // log_i("queued pad #%d clear in %dms", pad->index, feedbackDelay);
                }
            }
            return true;
        }

        case Touch::Event::tripleTouch: {
            log_i("pad %d triple", pad->index);
            return true;
        }

        case Touch::Event::quadrupleTouch: {
            log_i("pad %d quadruple", pad->index);
            return true;
        }

        case Touch::Event::quintupleTouch: {
            log_i("pad %d quintuple", pad->index);
            return true;
        }

        case Touch::Event::longTouch: {
            log_i("pad %d long", pad->index);
            fill(a, fg);
            if (queue([this, a]() { fill(a, bg, true); }, feedbackDelay)) {
                // log_i("queued pad #%d clear in %dms", pad->index, feedbackDelay);
            }
            return true;
        }

        case Touch::Event::touching: {
            // log_i("pad %d touching", pad->index);
            if (pad->start + Touch::longTouchTime < millis()) {  // animation completed, still touching
                fill(a, fg);
                if (queue([this, a]() { fill(a, bg, true); }, feedbackDelay)) {
                    // log_i("queued pad #%d clear in %dms", pad->index, feedbackDelay);
                }
                return true;
            }
            // log_i("pad %d animating", pad->index);
            Area b;
            memcpy(&b, a, sizeof(b));
            b.h = map(
                millis() - pad->start,
                0,
                Touch::longTouchTime,
                0,
                b.h);                     // scale down area height
            if (a->h < b.h) return true;  // overflow
            b.y += (a->h - b.h) / 2;      // move area to vertical middle
            fill(a, bg, false);
            fill(&b, fg);
            return true;
        }

        default: {
            log_e("unhandled %d", event);
        }
    }
    return true;
}

void Display::updateStatus(bool forceRedraw) {
    //  status area
    static const Area *a = &statusArea;

    static const uint8_t rideXbm[] = {
        0x00, 0x04, 0x80, 0x0a, 0x40, 0x05, 0xa0, 0x02, 0x50, 0x04, 0xa0, 0x08,
        0x50, 0x15, 0x80, 0x00, 0x5e, 0x1e, 0xb3, 0x33, 0x61, 0x21, 0x21, 0x21,
        0x33, 0x33, 0x1e, 0x1e};

    static const uint8_t walkXbm[] = {
        0x00, 0x00, 0x40, 0x00, 0xe0, 0x00, 0x60, 0x00, 0x70, 0x00, 0xa0, 0x00,
        0x50, 0x00, 0xa8, 0x01, 0x48, 0x01, 0x68, 0x02, 0x70, 0x00, 0xd8, 0x00,
        0x8c, 0x00, 0x84, 0x01};

    static const uint8_t standXbm[] = {
        0x00, 0x00, 0xe0, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x50, 0x01, 0xb0, 0x01,
        0x50, 0x01, 0xb0, 0x01, 0x50, 0x01, 0xa0, 0x00, 0xa0, 0x00, 0xa0, 0x00,
        0xa0, 0x00, 0xb0, 0x01};

    static const uint8_t recXbm[] = {
        0xe0, 0x01, 0xf8, 0x07, 0xfc, 0x0f, 0xfe, 0x1f, 0x3e, 0x1b, 0x1f, 0x32,
        0xcf, 0x34, 0xcf, 0x34, 0x1f, 0x36, 0x3e, 0x17, 0xfe, 0x17, 0xfc, 0x07,
        0xf8, 0x07, 0xe0, 0x01};

    static const uint8_t wifiXbm[] = {
        0x00, 0x00, 0xe0, 0x01, 0x38, 0x07, 0x0c, 0x0c, 0xc6, 0x18, 0xf3, 0x33,
        0x19, 0x26, 0xcc, 0x0c, 0xe4, 0x09, 0x30, 0x03, 0xc0, 0x00, 0xe0, 0x01,
        0xe0, 0x01, 0xc0, 0x00};

    static Area icon = Area(a->x, a->y, statusIconSize, statusIconSize);

    static int8_t lastMotionState = -1;

    static int8_t lastWifiState = -1;

    static int8_t lastRecordingState = -1;
    // 0: not recording, 1: recording but no gps fix yet, 2: recording
    int8_t recordingState = board.recorder.isRecording ? board.gps.locationIsValid() ? 2 : 1 : 0;

    if (!forceRedraw &&
        motionState == lastMotionState &&
        wifiState == lastWifiState &&
        1 != wifiState &&
        recordingState == lastRecordingState &&
        1 != recordingState)
        return;

    if (!aquireMutex()) return;

    icon.x = a->x;
    if (recordingState != lastRecordingState || 1 == recordingState || forceRedraw) {
        fillUnrestricted(&icon, bg, false);
        static bool recordingBlinkState = false;
        if (2 == recordingState || (1 == recordingState && recordingBlinkState))
            drawXBitmap(icon.x, icon.y, icon.w, icon.h, recXbm, fg, false);
        if (1 == recordingState) recordingBlinkState = !recordingBlinkState;
    }

    icon.x = a->x + a->w / 2 - statusIconSize / 2;
    if (wifiState != lastWifiState || 1 == wifiState || forceRedraw) {
        fillUnrestricted(&icon, bg, false);
        static bool wifiBlinkState = false;
        if (2 == wifiState || (1 == wifiState && wifiBlinkState))
            drawXBitmap(icon.x, icon.y, icon.w, icon.h, wifiXbm, fg, false);
        if (1 == wifiState) wifiBlinkState = !wifiBlinkState;
    }

    icon.x = a->x + a->w - statusIconSize;
    if (motionState != lastMotionState || forceRedraw) {
        fillUnrestricted(&icon, bg, false);
        drawXBitmap(icon.x, icon.y, icon.w, icon.h,
                    2 == motionState
                        ? rideXbm
                    : 1 == motionState
                        ? walkXbm
                        : standXbm,
                    fg, false);
    }
    sendBuffer();
    releaseMutex();

    lastMotionState = motionState;
    lastWifiState = wifiState;
    lastRecordingState = recordingState;
}

void Display::onWifiStateChange() {
    wifiState = board.wifi.isEnabled()
                    ? board.wifi.isConnected()
                          ? 2
                          : 1
                    : 0;
}

void Display::onPasChange() {
    if (!queue([this]() {
            if (!aquireMutex()) {
                log_d("could not aquire mutex");
                return;
            }
            enabled = true;
            uint16_t savedFg = fg;
            uint16_t savedBg = bg;
            setColor(pasFg());
            setBgColor(pasBg());
            char out[4];
            snprintf(out, 4, "%c%2d", board.pasMode == PAS_MODE_PROPORTIONAL ? 'P' : 'C', board.pasLevel);
            printField(0, out, false);
            setColor(savedFg);
            setBgColor(savedBg);
            sendBuffer();
            releaseMutex();
        },
               0, "displayPasLevel")) {
        log_d("could not queue displayPasLevel");
        return;
    }
    enabled = false;
    unqueue("displayFieldContent 0");
    if (!queue([this]() {
            enabled = true;
            displayFieldContent(0, field[0].content[currentPage]);
        },
               3000, "displayFieldContent 0")) {
        log_d("could not queue displayFieldContent 0");
        enabled = true;
    }
}

void Display::logArea(Area *a, Area *b, const char *str, const char *areaType) const {
    if (b->equals(a))
        log_i("%s %s", str, areaType);
    else if (b->contains(a))
        log_i("%s contains %s", str, areaType);
    else if (b->touches(a))
        log_i("%s touches %s", str, areaType);
}

void Display::logAreas(Area *a, const char *str) const {
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

bool Display::queue(QueueItemCallback callback, uint16_t delayMs, const char *tag) {
    if (0 == delayMs) {
        // log_w("delay is zero, executing callback");
        callback();
        return true;
    }
    ulong t = millis();

    if (0 == uxQueueSpacesAvailable(_queue)) {
        log_w("queue is full");
        return false;
    }
    QueueItem item;
    item.after = t + delayMs;
    item.callback = callback;
    if (nullptr != tag) strncpy(item.tag, tag, sizeof(item.tag));
    if (pdPASS != xQueueSendToBack(_queue, (void *)&item, 0)) {
        log_e("could not insert item into queue");
        return false;
    }
    // log_i("queued item %dms", delayMs);
    if (t + delayMs < taskGetNextWakeTimeMs()) {
        // log_i("next task wake is in %dms, setting task delay to %dms",
        //       taskGetNextWakeTimeMs() - t, delayMs);
        taskSetDelay(delayMs);
    }
    return true;
}

bool Display::queue(QueueItem item) {
    return queue(item.callback, item.after - millis());
}

int Display::unqueue(const char *tag) {
    if (nullptr == tag) {
        log_e("tag is null");
        return -1;
    }
    int removed = 0;
    for (UBaseType_t i = 0; i < uxQueueMessagesWaiting(_queue); i++) {
        QueueItem item;
        if (pdPASS != xQueueReceive(_queue, &(item), 0)) {
            log_e("failed to get item from queue");
            continue;
        }
        if (0 == strncmp(tag, item.tag, sizeof(item.tag) - 1)) {
            removed++;
        }
        queue(item);
    }
    return removed;
}

void Display::taskStart(float freq,
                        uint32_t stack,
                        int8_t priority,
                        int8_t core) {
    _taskSetFreqAndDelay(freq);
    defaultTaskFreq = _taskFreq;
    defaultTaskDelay = _taskDelay;
    Atoll::Task::taskStart(freq, stack, priority, core);
}

void Display::onLockChanged(bool locked) {
    /*
    uint8_t i;
    uint16_t oldColor = locked ? unlockedFg() : lockedFg();
    uint16_t newColor = locked ? lockedFg() : unlockedFg();
    for (i = 0; i < numFeedback; i++) {
        lockedFeedback(i, oldColor, 0);
    }
    if (queue([this, newColor]() {
            for (uint8_t i = 0; i < numFeedback; i++) {
                lockedFeedback(i, newColor, 0);
            }
        },
              200)) {
        // log_i("queued lockedFeedback all in %dms", 200);
    }
    if (queue([this]() {
            for (uint8_t i = 0; i < numFeedback; i++) {
                fill(&feedback[i], bg);
            }
        },
              500)) {
        // log_i("queued lockedFeedback all clear in %dms", 500);
    }
    */
    fill(&field[0].area, locked ? lockedBg() : unlockedBg());
    uint16_t savedFg = getColor();
    setColor(locked ? lockedFg() : unlockedFg());
    message(locked ? "locked" : "unlocked");
    setColor(savedFg);
    enabled = false;
    if (!queue([this]() {
            enabled = true;
            displayFieldContent(0, field[0].content[currentPage]);
        },
               3000)) {
        log_w("could not queue displayFieldContent");
        enabled = true;
    }
}

void Display::lockedFeedback(uint8_t padIndex, uint16_t color, uint16_t delayMs) {
    if (!aquireMutex()) return;
    assert(padIndex < sizeof(feedback) / sizeof(feedback[0]));
    Area *a = &feedback[padIndex];
    setClip(a);
    fill(a, bg, false);
    uint8_t stripeHeight = 2;
    for (int16_t y = a->y; y <= a->y + a->h - stripeHeight; y += stripeHeight * 2) {
        Area stripe(a->x, y, a->w, stripeHeight);
        fill(&stripe, color, false);
    }
    sendBuffer();
    setMaxClip();
    releaseMutex();
    if (0 < delayMs) {
        if (queue([this, a]() { fill(a, bg); }, delayMs)) {
            // log_i("queued pad #%d clear in %dms", padIndex, delayMs);
        }
    }
}

uint16_t Display::lockedFg() { return fg; }
uint16_t Display::lockedBg() { return bg; }
uint16_t Display::unlockedFg() { return fg; }
uint16_t Display::unlockedBg() { return bg; }
uint16_t Display::tareFg() { return bg; }
uint16_t Display::tareBg() { return fg; }
uint16_t Display::pasFg() { return bg; }
uint16_t Display::pasBg() { return fg; }

bool Display::aquireMutex(uint32_t timeout) {
    // log_d("aquireMutex %d", (int)mutex);
    // if (!enabled) {
    //     log_i("not enabled");
    //     return false;
    // }
    if (xSemaphoreTake(*mutex, (TickType_t)timeout) == pdTRUE)
        return true;
    log_i("could not aquire mutex");
    return false;
}

void Display::releaseMutex() {
    // log_d("releaseMutex %d", (int)mutex);
    xSemaphoreGive(*mutex);
}