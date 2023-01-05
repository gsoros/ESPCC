#include "board.h"
#include "ble_client.h"
#include "peers.h"

void BleClient::loadSettings() {
    if (!preferencesStartLoad()) return;
    for (uint8_t i = 0; i < peersMax; i++) {
        char key[8] = "";
        snprintf(key, sizeof(key), "peer%d", i);
        char packed[Peer::packedMaxLength] = "";
        strncpy(packed,
                preferences->getString(key, packed).c_str(),
                sizeof(packed));
        if (strlen(packed) < 1) continue;
        log_i("loading %s: %s", key, packed);
        Peer::Saved saved;
        if (Peer::unpack(packed, &saved)) {
            Peer *peer = createPeer(saved);
            if (nullptr == peer) continue;  // delete nullptr should be safe!
            if (!addPeer(peer)) delete peer;
        }
    }
    preferencesEnd();
}

void BleClient::saveSettings() {
    if (!preferencesStartSave()) return;
    char key[8] = "";
    char packed[Peer::packedMaxLength] = "";
    char saved[Peer::packedMaxLength] = "";
    for (uint8_t i = 0; i < peersMax; i++) {
        snprintf(key, sizeof(key), "peer%d", i);
        strncpy(packed, "", sizeof(packed));
        strncpy(saved,
                preferences->getString(key).c_str(),
                sizeof(saved));
        if (nullptr == peers[i] || peers[i]->markedForRemoval) {
            if (0 != strcmp(saved, "")) {
                log_i("removing %s", key);
                preferences->putString(key, "");
            }
        } else {
            if (!peers[i]->pack(packed, sizeof(packed))) {
                log_e("could not pack %s: %s", key, peers[i]->saved.address);
                continue;
            }
            if (0 != strcmp(saved, packed)) {
                log_i("saving %s: %s", key, packed);
                preferences->putString(key, packed);
            }
        }
    }
    preferencesEnd();
}

void BleClient::printSettings() {
    for (uint8_t i = 0; i < peersMax; i++)
        if (nullptr != peers[i])
            log_i("peer %d name: %s, type: %s, address: %s(%d), passkey: %d",
                  i, peers[i]->saved.name, peers[i]->saved.type, peers[i]->saved.address, peers[i]->saved.addressType, peers[i]->saved.passkey);
}

Peer *BleClient::createPeer(Peer::Saved saved) {
    log_d("creating %s,%d,%s,%s,%d", saved.address, saved.addressType, saved.type, saved.name, saved.passkey);
    Peer *peer;
    if (strstr(saved.type, "E"))
        peer = new ESPM(saved);
    else if (strstr(saved.type, "P"))
        peer = new PowerMeter(saved);
    else if (strstr(saved.type, "H"))
        peer = new HeartrateMonitor(saved);
    else if (strstr(saved.type, "V"))
        peer = new Vesc(saved);
    else
        return nullptr;
    return peer;
}

Peer *BleClient::createPeer(BLEAdvertisedDevice *device) {
    Peer::Saved saved;
    strncpy(saved.address, device->getAddress().toString().c_str(), sizeof(saved.address));
    saved.addressType = device->getAddress().getType();
    strncpy(saved.name, device->getName().c_str(), sizeof(saved.name));

    Peer *peer = nullptr;
    if (device->isAdvertisingService(BLEUUID(ESPM_API_SERVICE_UUID))) {
        strncpy(saved.type, "E", sizeof(saved.type));
        peer = new ESPM(saved);
    } else if (device->isAdvertisingService(BLEUUID(CYCLING_POWER_SERVICE_UUID))) {
        strncpy(saved.type, "P", sizeof(saved.type));
        peer = new PowerMeter(saved);
    } else if (device->isAdvertisingService(BLEUUID(HEART_RATE_SERVICE_UUID))) {
        strncpy(saved.type, "H", sizeof(saved.type));
        peer = new HeartrateMonitor(saved);
    } else if (device->isAdvertisingService(BLEUUID(VESC_SERVICE_UUID))) {
        strncpy(saved.type, "V", sizeof(saved.type));
        peer = new Vesc(saved);
    }
    return peer;
}

bool BleClient::startScan(uint32_t duration) {
    bool ret = Atoll::BleClient::startScan(duration);

    char reply[ATOLL_API_MSG_REPLY_LENGTH];
    snprintf(reply, sizeof(reply), "%d;%d=%d",
             ret ? board.api.success()->code : board.api.error()->code,
             board.api.command("scan")->code,
             (uint8_t)ret);

    log_d("calling bleServer.notify('api', 'tx', '%s', %d)", reply, strlen(reply));
    board.bleServer.notify(
        BLEUUID(ESPCC_API_SERVICE_UUID),
        BLEUUID(API_TX_CHAR_UUID),
        (uint8_t *)reply, strlen(reply));

    return ret;
}

bool BleClient::tarePowerMeter() {
    for (uint8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) {
            // log_i("peer #%d is null", i);
            continue;
        }
        if (!peers[i]->isConnected()) {
            log_i("peer #%d not connected", i);
            continue;
        }
        if (!peers[i]->isESPM()) {
            log_i("peer #%d not ESPM", i);
            continue;
        }
        ESPM *espm = (ESPM *)peers[i];
        if (espm->sendApiCommand("tare=0")) {
            log_i("peer #%d tare command sent");
            return true;
        }
    }
    log_i("could not send tare command");
    return false;
}

void BleClient::onResult(BLEAdvertisedDevice *device) {
    Atoll::BleClient::onResult(device);

    if (peerExists(device->getAddress().toString().c_str())) return;

    if (device->haveTargetAddress())
        for (int i = 0; i < device->getTargetAddressCount(); i++)
            log_i("Target address %i: %s", device->getTargetAddress(i).toString().c_str());

    Peer *peer = createPeer(device);
    if (nullptr == peer) {
        log_i("peer %s is null", device->getName().c_str());
        return;
    }
    // if (!addPeer(peer)) {
    //     delete peer;
    //     return ret;
    // }
    // else
    //     saveSettings();

    char reply[ATOLL_API_MSG_REPLY_LENGTH];
    snprintf(reply, sizeof(reply), "%d;%d=%s,%d,%s,%s",
             board.api.success()->code,
             board.api.command("scanResult")->code,
             peer->saved.address,
             peer->saved.addressType,
             peer->saved.type,
             peer->saved.name);

    log_i("calling bleServer.notify('api', 'tx', '%s', %d)", reply, strlen(reply));
    board.bleServer.notify(
        BLEUUID(ESPCC_API_SERVICE_UUID),
        BLEUUID(API_TX_CHAR_UUID),
        (uint8_t *)reply, strlen(reply));

    delete peer;
}

void BleClient::onScanEnd(BLEScanResults results) {
    Atoll::BleClient::onScanEnd(results);

    char reply[ATOLL_API_MSG_REPLY_LENGTH];
    snprintf(reply, sizeof(reply), "%d;%d=%d",
             board.api.success()->code,
             board.api.command("scan")->code,
             0);

    log_i("calling bleServer.notify('api', 'tx', '%s', %d)", reply, strlen(reply));
    board.bleServer.notify(
        BLEUUID(ESPCC_API_SERVICE_UUID),
        BLEUUID(API_TX_CHAR_UUID),
        (uint8_t *)reply, strlen(reply));
}