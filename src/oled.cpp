#include "oled.h"
#include "board.h"

void Oled::setup() {
    Atoll::Oled::setup();
    device->setBitmapMode(1);  // transparent
}

void Oled::loop() {
    if (board.recorder.isRecording)
        animateRecording();

    ulong t = millis();
    static ulong last = t;
    static uint16_t cutoff = 1000 - 1000 / taskFreq;
    if (t < last + cutoff)
        return;
    last = t;
    if ((lastPower < t - 5000) && (lastCadence < t - 5000)) {
        if (Atoll::systemTimeLastSet()) {
            showTime();
        } else
            showSatellites(1);
    }
    if (lastHeartrate < t - 3000) {
        if (Atoll::systemTimeLastSet()) {
            if (!board.gps.locationIsValid())
                showSatellites(2);
            else
                showDate();
        }
    }
}

void Oled::showSatellites(uint8_t fieldIndex) {
    printfField(fieldIndex, true, 1, 0, "-%02d", board.gps.satellites());
}

void Oled::animateRecording(bool clear) {
    static uint8_t riderSize = 14;  // width and height
    static Area track = {
        x : (uint8_t)(fields[1].x),
        y : (uint8_t)(fields[1].y + fields[1].h + fieldVSeparation / 2 - riderSize / 2),
        w : (uint8_t)(fields[1].w),
        h : (uint8_t)(riderSize)
    };
    static Area rider = {
        x : (uint8_t)(track.x - riderSize),  // start outside of the track
        y : (uint8_t)(track.y),
        w : (uint8_t)(riderSize),
        h : (uint8_t)(riderSize)
    };
    static const uint8_t riderBmp[] = {
        0x00, 0x04, 0x80, 0x0a, 0x40, 0x05, 0xa0, 0x02, 0x50, 0x04, 0xa0, 0x08,
        0x50, 0x15, 0x80, 0x00, 0x5e, 0x1e, 0xb3, 0x33, 0x61, 0x21, 0x21, 0x21,
        0x33, 0x33, 0x1e, 0x1e};
    static uint8_t fg;
    static uint8_t bg;

    if (board.gps.locationIsValid()) {
        fg = Color::FG;
        bg = Color::BG;
    } else {
        fg = Color::BG;
        bg = Color::FG;
    }

    if (!aquireMutex()) return;
    device->setDrawColor(bg);
    device->drawBox(track.x, track.y, track.w, track.h);
    if (clear) {
        device->sendBuffer();
        releaseMutex();
        return;
    }
    rider.x += 1;
    if (track.x + track.w + rider.w <= rider.x) rider.x = track.x;
    device->setDrawColor(fg);
    device->setClipWindow(track.x, track.y, track.x + track.w, track.y + track.h);
    device->drawXBM(rider.x - rider.w, rider.y, rider.w, rider.h, riderBmp);
    device->sendBuffer();
    device->setMaxClipWindow();
    releaseMutex();
}