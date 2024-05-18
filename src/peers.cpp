#include "board.h"
#include "peers.h"

void PowerMeter::onDisconnect(BLEClient* client, int reason) {
    Atoll::PowerMeter::onDisconnect(client, reason);
    board.display.onPMDisconnected();
    board.recorder.onPMDisconnected();
}

void ESPM::onDisconnect(BLEClient* client, int reason) {
    Atoll::ESPM::onDisconnect(client, reason);
    board.display.onPMDisconnected();
}

void HeartrateMonitor::onDisconnect(BLEClient* client, int reason) {
    Atoll::HeartrateMonitor::onDisconnect(client, reason);
    board.display.onHRMDisconnected();
    board.recorder.onHRMDisconnected();
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
                ((Vesc*)peer)->onHumanPower(lastValue);
            }
        }
    }
}

ApiTxChar::ApiTxChar() {
    PeerCharacteristicApiTX();
    strncpy(commands[0], "invalid", sizeof(commands[0]));
    strncpy(commands[1], "init", sizeof(commands[1]));
    for (uint8_t i = 2; i < sizeof(commands) / sizeof(commands[0]); i++)
        strncpy(commands[i], "", sizeof(commands[i]));
}

void ApiTxChar::notify() {
    Atoll::PeerCharacteristicApiTX::notify();
    processApiReply(lastValue.c_str());
}

void ApiTxChar::processApiReply(const char* reply) {
    // log_d("processing %s", reply);

    const char* cur = reply;

    const uint8_t success = 1;
    char search[3];
    snprintf(search, sizeof(search), "%d;", success);
    size_t searchLen = strlen(search);
    if (0 != strncmp(search, cur, searchLen)) {
        log_d("no success");
        return;
    }
    cur += searchLen;

    const char* eq = strchr(cur, '=');
    if (!eq) {
        log_d("no eq");
        return;
    }

    char codeStr[3];
    size_t codeStrLen = eq - cur;
    if (sizeof(codeStr) - 1 < codeStrLen) codeStrLen = sizeof(codeStr) - 1;
    snprintf(codeStr, codeStrLen + 1, "%s", cur);
    int code = atoi(codeStr);
    // log_d("codeStr: %s, code: %d", codeStr, code);
    if (code < 1 || ATOLL_API_MAX_COMMANDS - 1 < code) {
        log_e("command code %d out of range");
        return;
    }

    cur = eq + 1;

    if (code == commandCode("init")) {
        processInit(cur);
        // peer->getClient()->requestMTU(defaultMTU); not yet possible with nimbe_arduino
        return;
    }

    onApiReply(code, cur, strlen(cur));
}

void ApiTxChar::processInit(const char* value) {
    // 2:name2=value2;3:name3;4:name4=;5:name5=value5;...
    // log_d("processing init: %s", value);
    size_t len = strlen(value);
    if (len < 2) {
        log_i("init too short");
        return;
    }
    const char* cur = value;
    const char* semi;
    if (!semi) semi = cur + len;
    const char* colon;
    char codeStr[3];
    size_t codeStrLen;
    int code;
    const char* name;
    size_t nameLen;
    const char* eq;
    do {
        semi = strchr(cur, ';');
        if (!semi) semi = cur + strlen(cur);
        colon = strchr(cur, ':');
        if (!colon || semi < colon) goto cont;
        codeStrLen = colon - cur;
        if (sizeof(codeStr) - 1 < codeStrLen) codeStrLen = sizeof(codeStr) - 1;
        snprintf(codeStr, codeStrLen + 1, "%s", cur);
        code = atoi(codeStr);
        if (code < 2 || ATOLL_API_MAX_COMMANDS - 1 < code) {
            log_e("code %d out of range", code);
            goto cont;
        }
        eq = strchr(colon + 1, '=');
        if (semi < eq) eq = nullptr;
        nameLen = eq ? eq - colon - 1 : semi - colon - 1;
        if (nameLen < 1 || sizeof(commands[0]) - 1 < nameLen) {
            log_e("name length out of range: %d", nameLen);
            goto cont;
        }
        name = colon + 1;
        snprintf(commands[code], nameLen + 1, "%s", name);
        // log_d("added command %d: %s", code, commands[code]);
        if (!eq || 0 == strncmp("system", name, nameLen)) goto cont;
        onApiReply(code, eq + 1, semi - eq - 1);

    cont:
        cur = semi + 1;
    } while (semi = strchr(cur, ';'));

    // init is complete, this would be the time to request a small MTU
    // but it is not (yet) possible with nimble_arduino
    // if (nullptr != peer && peer->isESPM() && ((ESPM*)peer)->defaultMTU)
    // Atoll::Ble::setMTU(((ESPM*)peer)->defaultMTU);
}

void ApiTxChar::onApiReply(uint8_t code, const char* value, size_t len) {
    if (code < 2 || ATOLL_API_MAX_COMMANDS - 1 < code) {
        log_e("command code %d out of range");
        return;
    }

    if (code == commandCode("tare")) {
        log_d("processing tare");
        board.display.onTare();
        return;
    }
    if (code == commandCode("bat")) {
        // log_d("processing bat, value: \"%s\", len: %d", value, len);
        const char* sep = strchr(value, '|');
        const char charging[] = "charging";
        const char discharging[] = "discharging";
        if (nullptr != sep) {
            if (sep + sizeof(charging) - 1 < value + len && 0 == strncmp(charging, sep + 1, sizeof(charging) - 1)) {
                board.display.onBattPMState(Battery::ChargingState::csCharging);
                log_d("charging");
            } else if (sep + sizeof(discharging) - 1 < value + len && 0 == strncmp(discharging, sep + 1, sizeof(discharging) - 1)) {
                board.display.onBattPMState(Battery::ChargingState::csDischarging);
                log_d("discharging");
            }
        }
        char vstr[5] = "";
        size_t vlen = sep ? sep - value : strlen(value);
        if (len < vlen) vlen = len;
        if (sizeof(vstr) - 1 < vlen) vlen = sizeof(vstr) - 1;
        snprintf(vstr, vlen + 1, "%s", value);
        // log_d("vstr: %s", vstr);
        float voltage = (float)atof(vstr);
        if (ATOLL_BATTERY_EMPTY <= voltage && voltage <= ATOLL_BATTERY_FULL) {
            log_d("voltage: %.2f", voltage);
        }
    }
}

uint8_t ApiTxChar::commandCode(const char* name) {
    if (!strlen(name)) {
        log_e("empty name");
        return 0;
    }

    for (uint8_t i = 1; i < sizeof(commands) / sizeof(commands[0]); i++)
        if (0 == strcmp(name, commands[i])) return i;

    // char debug[256] = "";
    // snprintf(debug, sizeof(debug), "%s not found in ", name);
    // char cur[20] = "";
    // size_t curLen;
    // for (uint8_t i = 1; i < sizeof(commands) / sizeof(commands[0]); i++) {
    //     snprintf(cur, sizeof(cur), "%s, ", i, commands[i]);
    //     if (strlen(debug) + strlen(cur) < sizeof(debug) - 1)
    //         strcat(debug, cur);
    // }
    // log_e("%s", debug);

    return 0;
}

void WeightChar::notify() {
    Atoll::PeerCharacteristicWeightscale::notify();
    //log_i("WeightChar %2.2f", lastValue);
    board.display.onWeight(lastValue);
}

void TemperatureChar::notify() {
    Atoll::PeerCharacteristicTemperature::notify();
    // log_i("TemperatureChar %2.2f", lastValue);
    board.display.onTemperature(lastValue);
    board.recorder.onTemperature((int16_t)(lastValue * 100));
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
    uint8_t battLevel = Atoll::Battery::calculateLevel(uart->data.inpVoltage, board.vescSettings.battNumSeries);
    if (INT8_MAX < battLevel) battLevel = INT8_MAX;
    board.display.onBattVesc((int8_t)battLevel);
    if (board.gps.device.speed.isValid() &&
        0.0f < uart->data.avgInputCurrent &&
        0.0f < uart->data.inpVoltage) {
        float range = (float)board.gps.device.speed.kmph() * board.vescSettings.battCapacityWh * (float)battLevel / 100.0f / (uart->data.avgInputCurrent * uart->data.inpVoltage);
        if (INT16_MAX < range)
            range = INT16_MAX;
        else if (range < 0.0f)
            range = 0.0f;
        log_d("%.1fkm/h * %.0fWh * %d%% / (%.2fA * %.2fV) = %.0fkm",
              board.gps.device.speed.kmph(),
              board.vescSettings.battCapacityWh,
              battLevel,
              uart->data.avgInputCurrent,
              uart->data.inpVoltage,
              range);
        board.display.onRange((int16_t)range);
    } else {
        board.display.onRange(INT16_MAX);  // infinite range
    }
    // log_d("motor temp: %.2f˚C", uart->data.tempMotor);
    if (0 < board.vescSettings.tMotorWarn && board.vescSettings.tMotorWarn <= uart->data.tempMotor) {
        board.display.onMotorTemperature(uart->data.tempMotor);
    }
    // log_d("vesc temp: %.2f˚C", uart->data.tempMosfet);
    if (0 < board.vescSettings.tEscWarn && board.vescSettings.tEscWarn <= uart->data.tempMosfet) {
        board.display.onVescTemperature(uart->data.tempMosfet);
    }
}

void Vesc::setPower(uint16_t power) {
    static float prevCurrent = 0.0f;

    if (board.vescSettings.maxPower < power) power = board.vescSettings.maxPower;
    float voltage = getVoltage();
    if (voltage <= 0.01f) {
        log_e("voltage is 0");
        return;
    }
    if (1000.0f < voltage) {
        log_e("voltage too high");
        return;
    }
    float current = (float)(power / voltage);
    if (power <= 10)
        current = 0.0f;
    else if (current < board.vescSettings.minCurrent)
        current = board.vescSettings.minCurrent;
    else if (board.vescSettings.maxCurrent < current)
        current = board.vescSettings.maxCurrent;
    if (0.0f < current && 0 < board.vescSettings.rampNumSteps && board.vescSettings.rampMinCurrentDiff <= abs(current - prevCurrent)) {
        if (board.vescSettings.rampUp && prevCurrent < current) {
            if (prevCurrent < board.vescSettings.minCurrent)
                prevCurrent = board.vescSettings.minCurrent;
            float rampUnit = (current - prevCurrent) / board.vescSettings.rampNumSteps;
            uint16_t rampD = rampDelay();
            for (uint8_t i = 0; i < board.vescSettings.rampNumSteps; i++) {
                float rampCurrent = prevCurrent + rampUnit * i;
                log_d("setting ramp up current #%d: %2.2fA", i, rampCurrent);
                uart->setCurrent(rampCurrent);
                delay(rampD);
            }
        } else if (board.vescSettings.rampDown && current < prevCurrent) {
            float rampUnit = (prevCurrent - current) / board.vescSettings.rampNumSteps;
            uint16_t rampD = rampDelay();
            for (uint8_t i = board.vescSettings.rampNumSteps; 0 < i; i--) {
                float rampCurrent = current + rampUnit * i;
                log_d("setting ramp down current #%d: %2.2fA", i, rampCurrent);
                uart->setCurrent(rampCurrent);
                delay(rampD);
            }
        }
    }
    log_d("setting current: %2.2fA (%dW)", current, power);
    uart->setCurrent(current);
    prevCurrent = current;
}

void Vesc::onConnect(BLEClient* client) {
    Atoll::Vesc::onConnect(client);
    board.display.onVescConnected();
}

void Vesc::onDisconnect(BLEClient* client, int reason) {
    Atoll::Vesc::onDisconnect(client, reason);
    board.display.onVescDisconnected();
}

void Vesc::onHumanPower(uint16_t humanPower) {
    if (humanPower < board.pasMinHumanPower) return;

    if ((0 < board.vescSettings.tMotorLimit && board.vescSettings.tMotorLimit <= uart->data.tempMotor)  //
        ||
        (0 < board.vescSettings.tEscLimit && board.vescSettings.tEscLimit <= uart->data.tempMosfet)) {
        log_d("temperature limit exceeded, motor: %.2f˚C, esc: %.2f˚C, stopping %s",
              uart->data.tempMotor,
              uart->data.tempMosfet,
              saved.name);
        setPower(0);
        return;
    }

    uint32_t power = 0;
    if (PAS_MODE_CONSTANT == board.pasMode) {
        power = board.pasLevel * board.pasConstantFactor;
    } else {  // PAS_MODE_PROPORTIONAL == board.pasMode
        power = humanPower * board.pasLevel * board.pasProportionalFactor;
    }

    if (UINT16_MAX < power) power = UINT16_MAX;

    log_d("setting power to %d on %s (PAS: %c%d, human: %dW)",
          power,
          saved.name,
          PAS_MODE_CONSTANT == board.pasMode ? 'C' : 'P',
          board.pasLevel,
          humanPower);
    setPower((uint16_t)power);
}

uint16_t Vesc::rampDelay() {
    return board.vescSettings.rampNumSteps ? board.vescSettings.rampTime / board.vescSettings.rampNumSteps : (uint16_t)0;
}
