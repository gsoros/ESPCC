#include "board.h"
#include "recorder.h"

void Recorder::onDistanceChanged(double value) {
    Atoll::Recorder::onDistanceChanged(value);
    board.display.onDistance((uint)value);
}

void Recorder::onAltGainChanged(uint16_t value) {
    Atoll::Recorder::onAltGainChanged(value);
    board.display.onAltGain(value);
}

bool Recorder::rec2gpx(const char *in, const char *out) {
    bool res = Atoll::Recorder::rec2gpx(in, out);
    log_i("rec2gpx returned %d", res);
    if (res)
        board.webserver.generateIndex();
    return res;
}
