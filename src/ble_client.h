#ifndef __ble_client_h
#define __ble_client_h

#include "definitions.h"
#include "atoll_ble_client.h"
// #include "ble_server.h"

typedef Atoll::Peer Peer;
//  typedef Atoll::PowerMeter PowerMeter;

class BleClient : public Atoll::BleClient {
   public:
    virtual Peer* createPeer(Peer::Saved saved) override;
    virtual Peer* createPeer(BLEAdvertisedDevice* advertisedDevice) override;
    virtual bool tarePowerMeter();
};

#endif