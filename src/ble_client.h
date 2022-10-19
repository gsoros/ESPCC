#ifndef __ble_client_h
#define __ble_client_h

#include "definitions.h"
#include "atoll_ble_client.h"
#include "ble_server.h"

typedef Atoll::Peer Peer;
// typedef Atoll::PowerMeter PowerMeter;

class BleClient : public Atoll::BleClient {
   public:
    virtual Peer *createPeer(const char *address, uint8_t addressType, const char *type, const char *name);
    virtual Peer *createPeer(BLEAdvertisedDevice *advertisedDevice);

    virtual void loadSettings();
    virtual void saveSettings();
    virtual void printSettings();

    // duration is in milliseconds
    virtual uint32_t startScan(uint32_t duration) override;

    virtual bool tarePowerMeter();

   protected:
    virtual void onResult(BLEAdvertisedDevice *advertisedDevice) override;
    virtual void onScanEnd(BLEScanResults results) override;
};

#endif