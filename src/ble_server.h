#ifndef __ble_server_h
#define __ble_server_h

#include "definitions.h"
#include "atoll_ble_server.h"

class BleServer : public Atoll::BleServer {
   public:
    virtual void setup(const char *deviceName, ::Preferences *p) {
        Atoll::BleServer::setup(deviceName, p);
    }

    void onWrite(BLECharacteristic *c);
};

#endif