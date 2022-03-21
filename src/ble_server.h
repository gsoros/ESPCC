#ifndef __ble_server_h
#define __ble_server_h

#include "definitions.h"
#include "atoll_ble_constants.h"

#define BLE_APPEARANCE APPEARANCE_CYCLING_COMPUTER

#include "atoll_ble_server.h"

class BleServer : public Atoll::BleServer {
   public:
    virtual void setup(const char *deviceName, ::Preferences *p, const char *asUuidStr) {
        Atoll::BleServer::setup(deviceName, p, asUuidStr);
    }

    void onWrite(BLECharacteristic *c);
};

#endif