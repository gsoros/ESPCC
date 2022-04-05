#include "oled.h"
#include "board.h"

void Oled::setup() {
    Atoll::Oled::setup();
    device->setBitmapMode(1);  // transparent
}

void Oled::loop() {
    // if (board.recorder.isRecording)
    animateRecording();

    ulong t = millis();
    static ulong last = t;
    static uint16_t cutoff = 1000 - 1000 / taskFreq;
    if (t < last + cutoff)
        return;
    last = t;
    if (lastFieldUpdate < t - 5000 && Atoll::systemTimeLastSet())
        showTime();
}

void Oled::showSatellites(uint8_t fieldIndex) {
    printfFieldDigits(fieldIndex, true, C_FG, C_BG,
                      "-%02d", board.gps.satellites());
}

void Oled::animateRecording(bool clear) {
    static uint8_t riderSize = 14;  // width and height
    static Area track = {
        x : (uint8_t)(field[1].area.x),
        y : (uint8_t)(field[1].area.y +
                      field[1].area.h +
                      fieldVSeparation / 2 -
                      riderSize / 2),
        w : (uint8_t)(field[1].area.w),
        h : (uint8_t)(riderSize)
    };
    static Area rider = {
        x : (uint8_t)(track.x - riderSize),  // start outside of the track
        y : (uint8_t)(track.y),
        w : (uint8_t)(riderSize),
        h : (uint8_t)(riderSize)
    };
    static uint8_t lastX = rider.x;
    static const uint8_t riderXbm[] = {
        0x00, 0x04, 0x80, 0x0a, 0x40, 0x05, 0xa0, 0x02, 0x50, 0x04, 0xa0, 0x08,
        0x50, 0x15, 0x80, 0x00, 0x5e, 0x1e, 0xb3, 0x33, 0x61, 0x21, 0x21, 0x21,
        0x33, 0x33, 0x1e, 0x1e};
    static uint8_t fg;
    static uint8_t bg;

    if (board.gps.locationIsValid()) {
        fg = C_FG;
        bg = C_BG;
    } else {
        fg = C_BG;
        bg = C_FG;
    }

    if (board.gps.isMoving() &&
        board.recorder.isRecording)
        rider.x += 1;
    if (track.x + track.w + rider.w <= rider.x) rider.x = track.x;
    if (lastX == rider.x && !clear) return;
    if (!aquireMutex(300)) return;
    device->setDrawColor(bg);
    device->drawBox(track.x, track.y, track.w, track.h);
    if (clear) {
        device->sendBuffer();
        releaseMutex();
        return;
    }
    device->setDrawColor(fg);
    device->setClipWindow(track.x, track.y, track.x + track.w, track.y + track.h);
    device->drawXBM(rider.x - rider.w, rider.y, rider.w, rider.h, riderXbm);
    device->sendBuffer();
    device->setMaxClipWindow();
    releaseMutex();
    lastX = rider.x;
}

void Oled::onTouchEvent(Touch::Pad *pad, Touch::Event event) {
    Atoll::Oled::onTouchEvent(pad, event);
    if (Touch::Event::end == event) {
        currentPage++;
        if (numPages <= currentPage) currentPage = 0;
        log_i("currentPage %d", currentPage);
        if (!aquireMutex()) return;
        showTime(false, true);  // clear
        char buf[10] = "";
        for (uint8_t i = 0; i < numFields; i++)
            if (0 < fieldLabel(field[i].content[currentPage], buf, sizeof(buf)))
                printField(i, buf, false, labelFont);
        device->sendBuffer();
        delay(1000);
        for (uint8_t i = 0; i < numFields; i++)
            fill(&field[i].area, C_BG);
        releaseMutex();
        lastFieldUpdate = millis();
    }
}