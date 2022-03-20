#ifndef __ble_client_h
#define __ble_client_h

#include "definitions.h"
#include "atoll_ble_client.h"

typedef Atoll::Peer Peer;
// typedef Atoll::PowerMeter PowerMeter;

class BleClient : public Atoll::BleClient {
   public:
    virtual void setup(const char *deviceName, ::Preferences *p) {
        Atoll::BleClient::setup(deviceName, p);
    }

    virtual Peer *createPeer(const char *address, uint8_t addressType, const char *type, const char *name);
    virtual Peer *createPeer(BLEAdvertisedDevice *advertisedDevice);

    virtual void onResult(BLEAdvertisedDevice *advertisedDevice);

    virtual void loadSettings();
    virtual void saveSettings();
    virtual void printSettings();
};

#endif