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
    log_i("BattPMChar %d", lastValue);
    board.oled.onBattPM(lastValue);
}

void BattHRMChar::notify() {
    Atoll::PeerCharacteristicBattery::notify();
    log_i("BattHRMChar %d", lastValue);
    board.oled.onBattHRM(lastValue);
}

void PowerChar::notify() {
    Atoll::PeerCharacteristicPower::notify();
    // log_i("PowerChar %d %d", lastValue, lastCadence);
    board.oled.onPower(lastValue);
    board.oled.onCadence(lastCadence);
    board.recorder.onPower(lastValue);
    board.recorder.onCadence(lastCadence);
}

void WeightChar::notify() {
    Atoll::PeerCharacteristicWeightscale::notify();
    // log_i("WeightChar %2.2f", lastValue);
    board.oled.onWeight(lastValue);
}

void HeartrateChar::notify() {
    Atoll::PeerCharacteristicHeartrate::notify();
    // log_i("HeartrateChar %d", lastValue);
    board.oled.onHeartrate(lastValue);
    board.recorder.onHeartrate(lastValue);
}