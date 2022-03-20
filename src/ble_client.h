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

    virtual PeerDevice *createPeer(const char *address, const char *type, const char *name);
    virtual PeerDevice *createPeer(BLEAdvertisedDevice *advertisedDevice);

    virtual void onResult(BLEAdvertisedDevice *advertisedDevice);

    virtual void loadSettings();
    virtual void saveSettings();
    virtual void printSettings();
};

#endif