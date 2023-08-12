// #include "board.h"
#include "ble_client.h"
#include "peers.h"

bool BleClient::tarePowerMeter() {
    for (uint8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) {
            // log_i("peer #%d is null", i);
            continue;
        }
        if (!peers[i]->isConnected()) {
            log_d("peer #%d not connected", i);
            continue;
        }
        if (!peers[i]->isESPM()) {
            log_d("peer #%d not ESPM", i);
            continue;
        }
        ESPM *espm = (ESPM *)peers[i];
        if (espm->sendApiCommand("tare=0")) {
            log_d("peer #%d tare command sent", i);
            return true;
        }
    }
    log_i("could not send tare command");
    return false;
}
