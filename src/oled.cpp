#include "oled.h"
#include "board.h"

void Oled::setup() {
    Atoll::Oled::setup();
    // device->setFontMode(1);  // transparent
    // device->setBitmapMode(1);  // transparent
}

void Oled::loop() {
    displayStatus();

    if (lastFieldUpdate < millis() - 5000 && Atoll::systemTimeLastSet())
        showTime();
}

void Oled::displayStatus() {
    static const uint8_t iconSize = 14;  // width and height
    static const Area a = {
        x : (uint8_t)(field[1].area.x),
        y : (uint8_t)(field[1].area.y +
                      field[1].area.h +
                      fieldVSeparation / 2 -
                      iconSize / 2),
        w : (uint8_t)(field[1].area.w),
        h : (uint8_t)(iconSize)
    };

    static const uint8_t riderXbm[] = {
        0x00, 0x04, 0x80, 0x0a, 0x40, 0x05, 0xa0, 0x02, 0x50, 0x04, 0xa0, 0x08,
        0x50, 0x15, 0x80, 0x00, 0x5e, 0x1e, 0xb3, 0x33, 0x61, 0x21, 0x21, 0x21,
        0x33, 0x33, 0x1e, 0x1e};

    static const uint8_t recXbm[] = {
        0xe0, 0x01, 0xf8, 0x07, 0xfc, 0x0f, 0xfe, 0x1f, 0x3e, 0x1b, 0x1f, 0x32,
        0xcf, 0x34, 0xcf, 0x34, 0x1f, 0x36, 0x3e, 0x17, 0xfe, 0x17, 0xfc, 0x07,
        0xf8, 0x07, 0xe0, 0x01};

    Area icon = {
        x : (uint8_t)(a.x),
        y : (uint8_t)(a.y),
        w : (uint8_t)(iconSize),
        h : (uint8_t)(iconSize)
    };

    static bool lastIsMoving = false;
    bool isMoving = board.gps.isMoving();

    static bool lastIsRecording = false;
    bool isRecording = board.recorder.isRecording;

    if (isMoving == lastIsMoving &&
        isRecording == lastIsRecording)
        return;

    if (!aquireMutex()) return;
    if (isMoving != lastIsMoving) {
        fill(&icon, C_BG, false);
        if (isMoving) {
            device->setDrawColor(C_FG);
            device->drawXBM(icon.x, icon.y, icon.w, icon.h, riderXbm);
        }
    }
    icon.x = a.x + a.w - iconSize;
    if (isRecording != lastIsRecording) {
        fill(&icon, C_BG, false);
        if (isRecording) {
            device->setDrawColor(C_FG);
            device->drawXBM(icon.x, icon.y, icon.w, icon.h, recXbm);
            // device->drawDisc(icon.x + icon.w / 2, icon.y + icon.h / 2, icon.w / 2);
        }
    }
    device->sendBuffer();
    releaseMutex();

    lastIsMoving = isMoving;
    lastIsRecording = isRecording;
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
            showTime(false, true);  // clear
            char label[10] = "";
            Area *a;
            device->setFont(labelFont);
            for (uint8_t i = 0; i < numFields; i++) {
                if (fieldLabel(field[i].content[currentPage], label, sizeof(label)) < 1)
                    continue;
                a = &field[i].area;
                fill(a, C_BG, false);
                device->setCursor(a->x, a->y + a->h - 4);
                device->setDrawColor(C_FG);
                device->print(label);
            }
            device->sendBuffer();
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