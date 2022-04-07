#include "peers.h"
#include "board.h"

void PowerMeter::onDisconnect(BLEClient* client) {
    Atoll::PowerMeter::onDisconnect(client);
    board.oled.onPMDisconnected();
}

void HeartrateMonitor::onDisconnect(BLEClient* client) {
    Atoll::HeartrateMonitor::onDisconnect(client);
    board.oled.onHRMDisconnected();
}

void BattPMChar::notify() {
    Atoll::PeerCharacteristicBattery::notify();
    log_i("%d", lastValue);
    board.oled.onBattPM(lastValue);
}

void BattHRMChar::notify() {
    Atoll::PeerCharacteristicBattery::notify();
    log_i("%d", lastValue);
    board.oled.onBattHRM(lastValue);
}

void PowerChar::notify() {
    Atoll::PeerCharacteristicPower::notify();
    // log_i("PowerChar::onNotify %d %d", lastValue, lastCadence);
    board.oled.onPower(lastValue);
    board.oled.onCadence(lastCadence);
    board.recorder.onPower(lastValue);
    board.recorder.onCadence(lastCadence);
}

void HeartrateChar::notify() {
    Atoll::PeerCharacteristicHeartrate::notify();
    // log_i("HRChar::onNotify %d", lastValue);
    board.oled.onHeartrate(lastValue);
    board.recorder.onHeartrate(lastValue);
}