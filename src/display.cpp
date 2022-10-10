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
    _queue.clear();
    if (nullptr == mutex)
        defaultMutex = xSemaphoreCreateMutex();
    else
        this->mutex = mutex;
}

Display::~Display() {}

void Display::loop() {
    ulong t = millis();

    // static ulong lastLoopTime = 0;
    // log_i("loop %dms", lastLoopTime ? t - lastLoopTime : 0);
    // lastLoopTime = t;

    if (!_queue.isEmpty()) {
        ulong nextCallTime = 0;
        for (uint8_t i = 0; i < _queue.size(); i++) {
            QueueItem item = _queue.shift();
            if (item.after <= millis()) {
                // log_i("executing queued item #%d", i);
                item.callback();
            } else {
                // log_i("delaying queued item #%d", i);
                _queue.push(item);
                if (!nextCallTime || item.after < nextCallTime)
                    nextCallTime = item.after;
            }
        }
        if (_queue.isEmpty()) {
            // log_i("queue empty, setting default delay %dms", defaultTaskDelay);
            taskSetDelay(defaultTaskDelay);
        } else {
            // log_i("queue not empty");
            nextCallTime = min(nextCallTime, millis() + pdTICKS_TO_MS(defaultTaskDelay));
            // ulong nextWakeTime = taskGetNextWakeTimeMs();
            // if (nextCallTime < nextWakeTime) {
            if (nextCallTime) {
                uint16_t delay = 0;
                t = millis();
                if (t < nextCallTime) delay = nextCallTime - t;
                // log_i("setting delay %dms", delay);
                taskSetDelay(delay);
            }
        }
    }
    // log_i("queue run done");

    static ulong lastStatusUpdate = 0;
    t = millis();
    if (t - 1000 < lastStatusUpdate) return;
    lastStatusUpdate = t;
    updateStatus();

    if (lastFieldUpdate < t - 15000 && Atoll::systemTimeLastSet())
        clock();

    // static ulong lastFake = 0;
    // static double fake = 0.0;
    // if (lastFake + 500 < millis()) {
    //     fake += (double)random(0, 5000) / 10.0;
    //     onDistance((uint)fake);
    //     lastFake = millis();
    // }
}

size_t Display::print(const char *str) {
    if (board.otaMode) return 0;
    return printUnrestricted(str);
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

void Display::onSpeed(double value) {
    // log_i("%.2f", value);
    static double lastSpeed = 0;
    speed = value;
    if (lastSpeed == speed) return;
    displaySpeed();
    lastSpeed = speed;
    motionState = board.gps.minMovingSpeed <= speed
                      ? 10.0 < speed
                            ? 2
                            : 1
                      : 0;
}

bool Display::onTouchEvent(Touch::Pad *pad, Touch::Event event) {
    assert(pad->index < sizeof(feedback) / sizeof(feedback[0]));
    Area *a = &feedback[pad->index];
    static const uint16_t feedbackDelay = 600;

    if (board.touch.locked) {
        if (event == Touch::Event::singleTouch || event == Touch::Event::doubleTouch || event == Touch::Event::longTouch) {
            if (!aquireMutex()) return false;
            setClip(a);
            fill(a, bg, false);
            uint8_t stripeHeight = 2;
            for (int16_t y = a->y; y <= a->y + a->h - 2; y += stripeHeight * 2) {
                Area stripe(a->x, y, a->w, stripeHeight);
                fill(&stripe, fg, false);
            }
            sendBuffer();
            setMaxClip();
            releaseMutex();
            queue([this, a]() { fill(a, bg); }, 300);
            return false;
        }
        if (event == Touch::Event::quintupleTouch) return true;  // propagate to unlock
    }

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
            queue([this, a]() { fill(a, bg); }, 300);

            currentPage++;
            if (numPages <= currentPage) currentPage = 0;
            log_i("currentPage %d", currentPage);
            if (!aquireMutex()) return true;
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
            queue([this]() { displayFieldValues(); }, 3000);

            return false;  // do not propagate
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
                queue([this, a]() { fill(a, bg, true); }, feedbackDelay);  // clear
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
            queue([this, a]() { fill(a, bg, true); }, feedbackDelay);  // clear
            return true;
        }

        case Touch::Event::touching: {
            // log_i("pad %d touching", pad->index);
            if (pad->start + Touch::longTouchTime < millis()) {  // animation completed, still touching
                fill(a, fg);
                queue([this, a]() { fill(a, bg, true); }, feedbackDelay);  // clear
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

void Display::onWifiStateChange() {
    wifiState = board.wifi.isEnabled()
                    ? board.wifi.isConnected()
                          ? 2
                          : 1
                    : 0;
}