#include "board.h"
#include "recorder.h"

bool Recorder::start() {
    bool res = Atoll::Recorder::start();
    // log_d("start returned %d", res);
    notifyStatus();
    return res;
}

bool Recorder::stop(bool forgetLast) {
    bool res = Atoll::Recorder::stop(forgetLast);
    // log_d("stop returned %d", res);
    notifyStatus();
    return res;
}

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
    bool res = true;
    // bool res = Atoll::Recorder::rec2gpx(in, out, overwrite);
    // log_i("rec2gpx returned %d", res);
    log_i("rec2gpx is disabled");
#ifdef FEATURE_WEBSERVER
    if (res) board.webserver.generateIndex();
#endif
    return res;
}

void Recorder::notifyStatus() {
    BLECharacteristic *c = board.bleServer.getChar(BLEUUID(API_SERVICE_UUID), BLEUUID(API_TX_CHAR_UUID));
    if (!c) {
        log_e("could not get char");
        return;
    }
    ApiMessage m = board.api.process("rec");
    char reply[8 + strlen(m.reply)];
    snprintf(reply, sizeof(reply), "%d;%d=%s", m.result->code, m.commandCode, m.reply);
    log_d("notifying: %s", reply);
    c->setValue((uint8_t *)reply, strlen(reply));
    c->notify();
}