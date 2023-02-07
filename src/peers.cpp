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
    log_d("PowerChar %d %d", lastValue, lastCadence);
    board.display.onPower(lastValue);
    board.display.onCadence(lastCadence);
    board.recorder.onPower(lastValue);
    board.recorder.onCadence(lastCadence);
    if (0 < board.pasLevel) {
        for (uint8_t i = 0; i < board.bleClient.peersMax; i++) {
            Atoll::Peer* peer = board.bleClient.peers[i];
            if (nullptr != peer && peer->isVesc() && peer->isConnected()) {
                uint32_t power = lastValue * board.pasLevel;
                if (UINT16_MAX < power) power = UINT16_MAX;
                log_i("setting power to %d on %s (pasLevel: %d)", power, peer->saved.name, board.pasLevel);
                ((Vesc*)peer)->setPower((uint16_t)power);
            }
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
#ifdef FEATURE_SERIAL
    if (4 <= ATOLL_LOG_LEVEL) {  // 0: none, 1: error, 2: warning, 3, info, 4+: debug
        uart->setDebugPort(&board.hwSerial);
    }
#endif
}

void Vesc::loop() {
    Peer::loop();
    if (!requestUpdate()) return;
    uint8_t battLevel = Atoll::Battery::calculateLevel(uart->data.inpVoltage, 13);  // TODO get # of cells in series from settings
    if (INT8_MAX < battLevel) battLevel = INT8_MAX;
    board.display.onBattVesc((int8_t)battLevel);
    if (board.gps.device.speed.isValid() &&
        0.0f < uart->data.avgInputCurrent &&
        0.0f < uart->data.inpVoltage) {
        const float battCapacityWh = 740.0f;  // 3.7V * 20Ah, TODO get from settings
        float range = (float)board.gps.device.speed.kmph() * battCapacityWh * battLevel / (uart->data.avgInputCurrent * uart->data.inpVoltage);
        if (INT16_MAX < range)
            range = INT16_MAX;
        else if (range < 0.0f)
            range = 0;
        board.display.onRange((int16_t)range);
        log_d("%.1fkm/h * %.0fWh * %d%% / (%.2fA * %.2fV) = %.0fkm",
              board.gps.device.speed.kmph(),
              battCapacityWh,
              battLevel,
              uart->data.avgInputCurrent,
              uart->data.inpVoltage,
              range);
    } else
        board.display.onRange(INT16_MAX);  // infinite range
}

void Vesc::setPower(uint16_t power) {
    if (PAS_MODE_CONSTANT == board.pasMode && 50 < power) {
        power = board.pasLevel * 100;
        log_d("setting constant power %dW", power);
    }
    Atoll::Vesc::setPower(power);
}

void Vesc::onConnect(BLEClient* client) {
    Atoll::Vesc::onConnect(client);
    board.display.onVescConnected();
}

void Vesc::onDisconnect(BLEClient* client, int reason) {
    Atoll::Vesc::onDisconnect(client, reason);
    board.display.onVescDisconnected();
}