#include "recorder.h"
#include "board.h"

void Recorder::onDistanceChanged(double value) {
    board.oled.onDistance((uint)value);
}

void Recorder::onAltGainChanged(uint16_t value) {
    board.oled.onAltGain(value);
}

bool Recorder::rec2gpx(const char *in, const char *out) {
    bool res = Atoll::Recorder::rec2gpx(in, out);
    log_i("rec2gpx returned %d", res);
    if (res)
        board.recWebserver.generateIndex();
    return res;
}

bool Recorder::stop(bool forgetLast) {
    board.oled.animateRecording(true);
    return Atoll::Recorder::stop(forgetLast);
}