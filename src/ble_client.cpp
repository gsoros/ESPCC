// #include "board.h"
#include "ble_client.h"
#include "peers.h"

Peer* BleClient::createPeer(Peer::Saved saved) {  // make sure we use ESPCC::* and not Atoll::*
    if (strstr(saved.type, "E")) {
        return new ESPM(saved);
    }
    if (strstr(saved.type, "P")) {
        return new PowerMeter(saved);
    }
    if (strstr(saved.type, "H")) {
        return new HeartrateMonitor(saved);
    }
    if (strstr(saved.type, "V")) {
        return new Vesc(saved);
    }
    return Atoll::BleClient::createPeer(saved);
}

Peer* BleClient::createPeer(BLEAdvertisedDevice* advertisedDevice) {  // make sure we use ESPCC::* and not Atoll::*
    Peer::Saved saved = advertisedToSaved(advertisedDevice);
    if (advertisedDevice->isAdvertisingService(BLEUUID(ESPM_API_SERVICE_UUID))) {
        strncpy(saved.type, "E", sizeof(saved.type));
        return new ESPM(saved);
    }
    if (advertisedDevice->isAdvertisingService(BLEUUID(CYCLING_POWER_SERVICE_UUID))) {
        strncpy(saved.type, "P", sizeof(saved.type));
        return new PowerMeter(saved);
    }
    if (advertisedDevice->isAdvertisingService(BLEUUID(HEART_RATE_SERVICE_UUID))) {
        strncpy(saved.type, "H", sizeof(saved.type));
        return new HeartrateMonitor(saved);
    }
    if (advertisedDevice->isAdvertisingService(BLEUUID(VESC_SERVICE_UUID))) {
        strncpy(saved.type, "V", sizeof(saved.type));
        return new Vesc(saved);
    }

    return Atoll::BleClient::createPeer(advertisedDevice);
}

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
        ESPM* espm = (ESPM*)peers[i];
        if (espm->sendApiCommand("tare=0")) {
            log_d("peer #%d tare command sent", i);
            return true;
        }
    }
    log_i("could not send tare command");
    return false;
}
