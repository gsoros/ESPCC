#include "board.h"
#include "peers.h"

void PowerMeter::onDisconnect(BLEClient* client) {
    Atoll::PowerMeter::onDisconnect(client);
    board.display.onPMDisconnected();
}

void ESPM::onDisconnect(BLEClient* client) {
    Atoll::ESPM::onDisconnect(client);
    board.display.onPMDisconnected();
}

void HeartrateMonitor::onDisconnect(BLEClient* client) {
    Atoll::HeartrateMonitor::onDisconnect(client);
    board.display.onHRMDisconnected();
}

void BattPMChar::notify() {
    Atoll::PeerCharacteristicBattery::notify();
    // log_i("BattPMChar %d", lastValue);
    board.display.onBattPM(lastValue);
}

void BattHRMChar::notify() {
    Atoll::PeerCharacteristicBattery::notify();
    log_i("BattHRMChar %d", lastValue);
    board.display.onBattHRM(lastValue);
}

void PowerChar::notify() {
    Atoll::PeerCharacteristicPower::notify();
    // log_i("PowerChar %d %d", lastValue, lastCadence);
    board.display.onPower(lastValue);
    board.display.onCadence(lastCadence);
    board.recorder.onPower(lastValue);
    board.recorder.onCadence(lastCadence);
}

void WeightChar::notify() {
    Atoll::PeerCharacteristicWeightscale::notify();
    // log_i("WeightChar %2.2f", lastValue);
    board.display.onWeight(lastValue);
}

void HeartrateChar::notify() {
    Atoll::PeerCharacteristicHeartrate::notify();
    // log_i("HeartrateChar %d", lastValue);
    board.display.onHeartrate(lastValue);
    board.recorder.onHeartrate(lastValue);
}