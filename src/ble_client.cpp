#include "ble_client.h"
#include "board.h"
#include "powermeter.h"

void BleClient::onResult(BLEAdvertisedDevice* device) {
    log_i("scan found %s", device->toString().c_str());
    /*
    if (device->haveName())
        log_i("haveName: %s", device->getName().c_str());
    if (device->haveServiceData())
        log_i("haveServiceData: %s", device->getServiceData().c_str());
    if (device->haveServiceUUID())
        log_i("haveServiceUUID: %s", device->getServiceUUID().toString().c_str());
    log_i("getServiceDataCount: %d getServiceUUIDCount: %d getServiceDataUUID %s",
          device->getServiceDataCount(),
          device->getServiceUUIDCount(),
          device->getServiceDataUUID().toString().c_str());
    for (uint8_t i = 0; i < device->getServiceUUIDCount(); i++) {
        log_i("service %d: %s", i, device->getServiceUUID(i).toString().c_str());
        if (device->getServiceUUID(i).equals(espmUuid)) {
            log_i("getServiceUUID espmApiServiceUuid !!!!! +++");
            break;
        }
    }
    */
    BLEUUID espmUuid = BLEUUID(ESPM_API_SERVICE_UUID);
    BLEUUID cpsUuid = BLEUUID(CYCLING_POWER_SERVICE_UUID);
    BLEUUID hrsUuid = BLEUUID(HEART_RATE_SERVICE_UUID);
    char type[4] = "";
    if (device->isAdvertisingService(espmUuid)) {
        log_i("type ESPM");
        strcat(type, "E");
    }
    if (device->isAdvertisingService(cpsUuid)) {
        log_i("type power");
        strcat(type, "P");
    }
    if (device->isAdvertisingService(hrsUuid)) {
        log_i("type heartrate");
        strcat(type, "H");
    }
    if (strlen(type) < 1) return;

    char address[sizeof(PeerDevice::address)];
    strncpy(address, device->getAddress().toString().c_str(), sizeof(address));
    if (peerExists(address)) return;
    PeerDevice* peer;
    if (strstr(type, "P"))
        peer = new PowerMeter(
            address,
            type,
            device->getName().c_str());
    else
        peer = new PeerDevice(
            address,
            type,
            device->getName().c_str());
    if (!addPeer(peer)) delete peer;

    char value[ATOLL_API_VALUE_LENGTH];
    snprintf(value, sizeof(value), "%d;%d=%s,%s,%s",
             board.api.success()->code,
             board.api.command("scanResult")->code,
             address,
             type,
             device->getName().c_str());
    log_i("bleServer.setApiValue('%s')", value);
    board.bleServer.setApiValue(value);
}