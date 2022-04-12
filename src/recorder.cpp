#include "recorder.h"
#include "board.h"

void Recorder::onDistanceChanged(double value) {
    Atoll::Recorder::onDistanceChanged(value);
    board.oled.onDistance((uint)value);
}

void Recorder::onAltGainChanged(uint16_t value) {
    Atoll::Recorder::onAltGainChanged(value);
    board.oled.onAltGain(value);
}

bool Recorder::start() {
    bool res = Atoll::Recorder::start();

    if (res) {
        // disable wifi but don't save
        board.wifi.setEnabled(false, false);
        // stop webserver
        board.webserver.end();
    }
    return res;
}

bool Recorder::stop(bool forgetLast) {
    bool res = Atoll::Recorder::stop(forgetLast);
    // enable wifi but don't save
    if (forgetLast) board.wifi.setEnabled(true, false);
    return res;
}

bool Recorder::rec2gpx(const char *in, const char *out) {
    bool res = Atoll::Recorder::rec2gpx(in, out);
    log_i("rec2gpx returned %d", res);
    if (res)
        board.webserver.generateIndex();
    return res;
}
