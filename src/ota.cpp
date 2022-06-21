#include "ota.h"
#include "board.h"

void Ota::onStart() {
    Atoll::Ota::onStart();
    board.display.onOta("start");
    log_i("pausing recorder");
    board.recorder.pause();
}

void Ota::onEnd() {
    Atoll::Ota::onEnd();
    board.display.onOta("end");
}

void Ota::onProgress(uint progress, uint total) {
    Atoll::Ota::onProgress(progress, total);
    uint8_t percent = (uint8_t)((float)progress / (float)total * 100.0);
    static uint8_t lastPercent = 0;
    if (percent != lastPercent) {
        char out[5] = "";
        snprintf(out, sizeof(out), "%d%%", percent);
        board.display.onOta(out);
        lastPercent = percent;
    }
}

void Ota::onError(ota_error_t error) {
    Atoll::Ota::onError(error);
    char out[5] = "";
    snprintf(out, sizeof(out), "E%d", error);
    board.display.onOta(out);
}
