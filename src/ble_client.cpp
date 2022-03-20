#include "ble_client.h"
#include "board.h"
#include "powermeter.h"

void BleClient::loadSettings() {
    if (!preferencesStartLoad()) return;
    for (uint8_t i = 0; i < peersMax; i++) {
        char key[8] = "";
        snprintf(key, sizeof(key), "peer%d", i);
        char packed[PeerDevice::packedMaxLength] = "";
        strncpy(packed, preferences->getString(key, packed).c_str(), sizeof(packed));
        if (strlen(packed) < 1) continue;
        log_i("loading %s: %s", key, packed);
        char address[sizeof(PeerDevice::address)] = "";
        char type[sizeof(PeerDevice::type)] = "";
        char name[sizeof(PeerDevice::name)] = "";
        if (PeerDevice::unpack(
                packed,
                address, sizeof(address),
                type, sizeof(type),
                name, sizeof(name))) {
            PeerDevice *peer = createPeer(address, type, name);
            if (nullptr == peer) continue;  // delete nullptr should be safe!
            if (!addPeer(peer)) delete peer;
        }
    }
    preferencesEnd();
}

void BleClient::saveSettings() {
    if (!preferencesStartSave()) return;
    char key[8] = "";
    char packed[PeerDevice::packedMaxLength] = "";
    for (uint8_t i = 0; i < peersMax; i++) {
        snprintf(key, sizeof(key), "peer%d", i);
        strncpy(packed, "", sizeof(packed));
        if (nullptr == peers[i] || peers[i]->markedForRemoval)
            log_i("removing %s", key);
        else if (peers[i]->pack(packed, sizeof(packed)))
            log_i("saving %s: %s", key, packed);
        else
            log_e("could not pack %s: %s", key, peers[i]->address);
        preferences->putString(key, packed);
    }
    preferencesEnd();
}

void BleClient::printSettings() {
    for (uint8_t i = 0; i < peersMax; i++)
        if (nullptr != peers[i])
            log_i("peer %d name: %s, type: %s, address: %s",
                  i, peers[i]->name, peers[i]->type, peers[i]->address);
}

PeerDevice *BleClient::createPeer(const char *address, const char *type, const char *name) {
    PeerDevice *peer;
    //    if (strstr(type, "E"))
    //        peer = new ESPM(address,type,name);
    if (strstr(type, "P"))
        peer = new PowerMeter(address, type, name);
    //    else if (strstr(type, "H"))
    //        peer = new HeartrateMonitor(address, type, name);
    else
        return nullptr;
    return peer;
}

PeerDevice *BleClient::createPeer(BLEAdvertisedDevice *device) {
    // BLEUUID espmUuid = BLEUUID(ESPM_API_SERVICE_UUID);
    BLEUUID cpsUuid = BLEUUID(CYCLING_POWER_SERVICE_UUID);
    // BLEUUID hrsUuid = BLEUUID(HEART_RATE_SERVICE_UUID);
    char address[sizeof(PeerDevice::address)];
    strncpy(address, device->getAddress().toString().c_str(), sizeof(address));
    PeerDevice *peer = nullptr;
    //    if (device->isAdvertisingService(espmUuid))
    //        peer = new ESPM(address,"E",device->getName().c_str());
    if (device->isAdvertisingService(cpsUuid))
        peer = new PowerMeter(address, "P", device->getName().c_str());
    //    else if (device->isAdvertisingService(hrsUuid)) {
    //        peer = new HeartrateMonitor(address, "H", device->getName().c_str());
    return peer;
}

void BleClient::onResult(BLEAdvertisedDevice *device) {
    log_i("scan found %s", device->toString().c_str());

    if (peerExists(device->getAddress().toString().c_str())) return;
    PeerDevice *peer = createPeer(device);
    if (nullptr == peer) {  // delete nullptr should be safe!
        log_i("not adding peer %s", device->getName().c_str());
        return;
    }
    if (!addPeer(peer))
        delete peer;
    else
        saveSettings();

    char value[ATOLL_API_VALUE_LENGTH];
    snprintf(value, sizeof(value), "%d;%d=%s,%s,%s",
             board.api.success()->code,
             board.api.command("scanResult")->code,
             peer->address,
             peer->type,
             peer->name);
    log_i("bleServer.setApiValue('%s')", value);
    board.bleServer.setApiValue(value);
}