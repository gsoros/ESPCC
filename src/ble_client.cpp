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
        char address[sizeof(Peer::address)] = "";
        uint8_t addressType = 0;
        char type[sizeof(Peer::type)] = "";
        char name[sizeof(Peer::name)] = "";
        if (Peer::unpack(
                packed,
                address, sizeof(address),
                &addressType,
                type, sizeof(type),
                name, sizeof(name))) {
            Peer *peer = createPeer(address, addressType, type, name);
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
                log_e("could not pack %s: %s", key, peers[i]->address);
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
            log_i("peer %d name: %s, type: %s, address: %s(%d)",
                  i, peers[i]->name, peers[i]->type, peers[i]->address, peers[i]->addressType);
}

Peer *BleClient::createPeer(
    const char *address,
    uint8_t addressType,
    const char *type,
    const char *name) {
    // log_i("creating %s,%d,%s,%s", address, addressType, type, name);
    Peer *peer;
    if (strstr(type, "E"))
        peer = new ESPM(address, addressType, type, name);
    else if (strstr(type, "P"))
        peer = new PowerMeter(address, addressType, type, name);
    else if (strstr(type, "H"))
        peer = new HeartrateMonitor(address, addressType, type, name);
    else
        return nullptr;
    return peer;
}

Peer *BleClient::createPeer(BLEAdvertisedDevice *device) {
    char address[sizeof(Peer::address)];
    strncpy(address, device->getAddress().toString().c_str(), sizeof(address));
    uint8_t addressType = device->getAddress().getType();
    char name[sizeof(Peer::name)];
    strncpy(name, device->getName().c_str(), sizeof(name));

    Peer *peer = nullptr;
    if (device->isAdvertisingService(BLEUUID(ESPM_API_SERVICE_UUID)))
        peer = new ESPM(address, addressType, "E", name);
    else if (device->isAdvertisingService(BLEUUID(CYCLING_POWER_SERVICE_UUID)))
        peer = new PowerMeter(address, addressType, "P", name);
    else if (device->isAdvertisingService(BLEUUID(HEART_RATE_SERVICE_UUID)))
        peer = new HeartrateMonitor(address, addressType, "H", name);
    return peer;
}

uint32_t BleClient::startScan(uint32_t duration) {
    uint32_t ret = Atoll::BleClient::startScan(duration);

    char reply[ATOLL_API_MSG_REPLY_LENGTH];
    snprintf(reply, sizeof(reply), "%d;%d=%d",
             board.api.success()->code,
             board.api.command("scan")->code,
             ret);

    log_i("calling bleServer.notify('api', 'tx', '%s', %d)", reply, strlen(reply));
    board.bleServer.notify(
        BLEUUID(ESPCC_API_SERVICE_UUID),
        BLEUUID(API_TX_CHAR_UUID),
        (uint8_t *)reply, strlen(reply));

    return ret;
}

// overriden in order to call our version of static onScanComplete
bool BleClient::callScanStart(uint32_t duration) {
    return scan->start(duration, onScanComplete, false);
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
             peer->address,
             peer->addressType,
             peer->type,
             peer->name);

    log_i("calling bleServer.notify('api', 'tx', '%s', %d)", reply, strlen(reply));
    board.bleServer.notify(
        BLEUUID(ESPCC_API_SERVICE_UUID),
        BLEUUID(API_TX_CHAR_UUID),
        (uint8_t *)reply, strlen(reply));

    delete peer;
}

void BleClient::onScanComplete(BLEScanResults results) {
    Atoll::BleClient::onScanComplete(results);

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