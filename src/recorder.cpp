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

bool Recorder::rec2gpx(const char *in,
                       const char *out,
                       bool overwrite) {
    bool res = Atoll::Recorder::rec2gpx(in, out, overwrite);
    log_i("rec2gpx returned %d", res);
    #ifdef FEATURE_WEBSERVER
    if (res) board.webserver.generateIndex();
    #endif
    return res;
}
