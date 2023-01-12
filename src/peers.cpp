#include "board.h"
#include "peers.h"

void PowerMeter::onDisconnect(BLEClient* client, int reason) {
    Atoll::PowerMeter::onDisconnect(client, reason);
    board.display.onPMDisconnected();
}

void ESPM::onDisconnect(BLEClient* client, int reason) {
    Atoll::ESPM::onDisconnect(client, reason);
    board.display.onPMDisconnected();
}

void HeartrateMonitor::onDisconnect(BLEClient* client, int reason) {
    Atoll::HeartrateMonitor::onDisconnect(client, reason);
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
    for (uint8_t i = 0; i < board.bleClient.peersMax; i++) {
        Atoll::Peer* peer = board.bleClient.peers[i];
        if (nullptr != peer && peer->isVesc() && peer->isConnected()) {
            log_i("setting power to %d on Vesc", lastValue);
            ((Vesc*)peer)->setPower(lastValue);  // TODO times factor
        }
    }
}

void ApiTxChar::notify() {
    Atoll::PeerCharacteristicApiTX::notify();
    if (lastValue == String("1;5=")) {  // TODO command 5=tare
        log_i("TODO process api replies");
        board.display.onTare();
    }
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

Vesc::Vesc(Atoll::Peer::Saved saved,
           Atoll::PeerCharacteristicVescRX* customVescRX,
           Atoll::PeerCharacteristicVescTX* customVescTX)
    : Atoll::Vesc(saved, customVescRX, customVescTX) {
    if (4 <= ATOLL_LOG_LEVEL) {  // 0: none, 1: error, 2: warning, 3, info, 4+: debug
        uart->setDebugPort(&board.hwSerial);
    }
}