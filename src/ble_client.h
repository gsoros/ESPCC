#ifndef __ble_client_h
#define __ble_client_h

#include "definitions.h"
#include "atoll_ble_client.h"

typedef Atoll::BlePeerDevice PeerDevice;
// typedef Atoll::PowerMeter PowerMeter;

class BleClient : public Atoll::BleClient {
   public:
    virtual void setup(const char *deviceName, ::Preferences *p) {
        Atoll::BleClient::setup(deviceName, p);
    }

    virtual void onResult(BLEAdvertisedDevice *advertisedDevice);
};

#endif