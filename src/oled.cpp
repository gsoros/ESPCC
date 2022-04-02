#include "oled.h"
#include "board.h"

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
            showSatellites();
    }
    if (lastHeartrate < t - 3000)
        if (Atoll::systemTimeLastSet())
            showDate();
}

void Oled::showSatellites() {
    printfField(1, true, 1, 0, "-%02d", board.gps.satellites());
}

void Oled::animateRecording(bool clear) {
    static Area track = {
        x : (uint8_t)(fields[1].x),
        y : (uint8_t)(fields[1].y + fields[1].h + 1),
        w : (uint8_t)(fields[1].w),
        h : (uint8_t)(14)
    };
    static Area rider = {
        x : (uint8_t)(track.x - track.h),
        y : (uint8_t)(track.y),
        w : (uint8_t)(track.h),
        h : (uint8_t)(track.h)
    };
    static const uint8_t riderBmp[] = {
        0x00, 0x04, 0x80, 0x0a, 0x40, 0x05, 0xa0, 0x02, 0x50, 0x04, 0xa0, 0x08,
        0x50, 0x15, 0x80, 0x00, 0x5e, 0x1e, 0xb3, 0x33, 0x61, 0x21, 0x21, 0x21,
        0x33, 0x33, 0x1e, 0x1e};  // 14 x 14px
    if (clear) {
        fill(&rider, 0);
        return;
    }

    // log_i("track{%d, %d, %d, %d}, rider{%d, %d, %d, %d}",  //
    //       track.x, track.y, track.w, track.h,              //
    //       rider.x, rider.y, rider.w, rider.h);

    if (!aquireMutex()) return;
    device->setClipWindow(track.x, track.y, track.x + track.w, track.y + track.h);
    device->setDrawColor(0);
    device->drawBox(rider.x - rider.w, rider.y, rider.w, rider.h);
    rider.x += 1;
    if (track.x + track.w + rider.w <= rider.x) rider.x = track.x;
    device->setDrawColor(1);
    device->drawXBM(rider.x - rider.w, rider.y, rider.w, rider.h, riderBmp);
    device->sendBuffer();
    device->setMaxClipWindow();
    releaseMutex();
}