#include "oled.h"
#include "board.h"

void Oled::setup() {
    Atoll::Oled::setup();
    // device->setFontMode(1);  // transparent
    // device->setBitmapMode(1);  // transparent
    splash();
}

void Oled::loop() {
    displayStatus();

    if (lastFieldUpdate + 15000 < millis() && Atoll::systemTimeLastSet())
        displayClock();

    // static ulong lastFake = 0;
    // static double fake = 0.0;
    // if (lastFake + 500 < millis()) {
    //     fake += (double)random(0, 5000) / 10.0;
    //     onDistance((uint)fake);
    //     lastFake = millis();
    // }
}

void Oled::displayStatus() {
    static const uint8_t iconSize = 14;  // status icon width and height
    // status area
    static const Area a = {
        x : feedbackWidth,
        y : (uint8_t)(height - iconSize),
        w : fieldWidth,
        h : iconSize
    };

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

    static Area icon = {
        x : a.x,
        y : a.y,
        w : iconSize,
        h : iconSize
    };

    static int8_t lastMotionState = -1;

    static int8_t lastWifiState = -1;

    static int8_t lastRecordingState = -1;
    int8_t recordingState = (int8_t)board.recorder.isRecording;

    if (motionState == lastMotionState &&
        wifiState == lastWifiState &&
        1 != wifiState &&
        recordingState == lastRecordingState)
        return;

    if (!aquireMutex()) return;

    icon.x = a.x;
    if (recordingState != lastRecordingState) {
        fill(&icon, C_BG, false);
        if (recordingState) {
            device->setDrawColor(C_FG);
            device->drawXBM(icon.x, icon.y, icon.w, icon.h, recXbm);
        }
    }

    icon.x = a.x + a.w / 2 - iconSize / 2;
    if (wifiState != lastWifiState || 1 == wifiState) {
        fill(&icon, C_BG, false);
        static bool wifiBlinkState = false;
        if (2 == wifiState || (1 == wifiState && wifiBlinkState)) {
            device->setDrawColor(C_FG);
            device->drawXBM(icon.x, icon.y, icon.w, icon.h, wifiXbm);
        }
        if (1 == wifiState) wifiBlinkState = !wifiBlinkState;
    }

    icon.x = a.x + a.w - iconSize;
    if (motionState != lastMotionState) {
        fill(&icon, C_BG, false);
        device->setDrawColor(C_FG);
        device->drawXBM(icon.x, icon.y, icon.w, icon.h,
                        2 == motionState
                            ? rideXbm
                        : 1 == motionState
                            ? walkXbm
                            : standXbm);
    }
    device->sendBuffer();
    releaseMutex();

    lastMotionState = motionState;
    lastWifiState = wifiState;
    lastRecordingState = recordingState;
}

void Oled::onSpeed(double value) {
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

void Oled::onTouchEvent(Touch::Pad *pad, Touch::Event event) {
    assert(pad->index < sizeof(feedback) / sizeof(feedback[0]));
    Area *a = &feedback[pad->index];

    switch (event) {
        case Touch::Event::start: {
            return;
        }

        case Touch::Event::end: {
            return;
        }

        case Touch::Event::singleTouch: {
            log_i("pad %d single", pad->index);
            fill(a, C_FG);
            fill(a, C_BG, true, 1000);

            currentPage++;
            if (numPages <= currentPage) currentPage = 0;
            log_i("currentPage %d", currentPage);
            if (!aquireMutex()) return;
            displayClock(false, true);  // clear clock
            char label[10] = "";
            Area *a;
            device->setFont(labelFont);
            for (uint8_t i = 0; i < numFields; i++) {
                if (fieldLabel(field[i].content[currentPage], label, sizeof(label)) < 1)
                    continue;
                a = &field[i].area;
                device->setClipWindow(a->x, a->y, a->x + a->w, a->y + a->h);
                fill(a, C_BG, false);
                device->setCursor(a->x, a->y + a->h - 4);
                device->setDrawColor(C_FG);
                device->print(label);
            }
            device->sendBuffer();
            device->setMaxClipWindow();
            delay(1000);
            displayFieldValues(false);
            device->sendBuffer();
            releaseMutex();
            lastFieldUpdate = millis();

            return;
        }

        case Touch::Event::doubleTouch: {
            log_i("pad %d double", pad->index);
            Area b;
            memcpy(&b, a, sizeof(b));
            b.h /= 3;  // divide area height by 3
            if (aquireMutex()) {
                fill(&b, C_FG, false);  // area 1 white
                b.y += b.h;             // move down
                fill(&b, C_BG, false);  // area 2 black
                b.y += b.h;             // move down
                fill(&b, C_FG, false);  // area 3 white
                device->sendBuffer();
                releaseMutex();
            }
            delay(Touch::touchTime * 3);  // TODO is delay() a good idea? maybe create a queue schedule?
            fill(a, C_BG, true, 1000);    // clear
            return;
        }

        case Touch::Event::longTouch: {
            log_i("pad %d long", pad->index);
            fill(a, C_FG);
            delay(Touch::touchTime * 3);
            fill(a, C_BG, true, 1000);
            return;
        }

        case Touch::Event::touching: {
            // log_i("pad %d touching", pad->index);
            if (pad->start + Touch::longTouchTime < millis()) {  // animation completed, still touching
                fill(a, C_FG);
                delay(Touch::touchTime * 3);
                fill(a, C_BG, true, 1000);
                return;
            }
            // log_i("pad %d animating", pad->index);
            Area b;
            memcpy(&b, a, sizeof(b));
            b.h = map(
                millis() - pad->start,
                0,
                Touch::longTouchTime,
                0,
                b.h);                 // scale down area height
            if (a->h < b.h) return;   // overflow
            b.y += (a->h - b.h) / 2;  // move area to vertical middle
            fill(a, C_BG, false);
            fill(&b, C_FG);
            return;
        }

        default: {
            log_e("unhandled %d", event);
        }
    }
}

void Oled::onWifiEvent() {
    wifiState = board.wifi.isEnabled()
                    ? board.wifi.connected()
                          ? 2
                          : 1
                    : 0;
}