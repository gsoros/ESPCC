#ifndef __peers_h
#define __peers_h

#include "atoll_peer.h"
#include "atoll_peer_characteristic.h"

class PowerChar : public Atoll::PeerCharacteristicPower {
    void onNotify(
        BLERemoteCharacteristic* c,
        uint8_t* data,
        size_t length,
        bool isNotify);
};

class PowerMeter : public Atoll::PowerMeter {
   public:
    PowerMeter(
        const char* address,
        uint8_t addressType,
        const char* type,
        const char* name)
        : Atoll::PowerMeter(
              address,
              addressType,
              type,
              name,
              new PowerChar()) {
    }
};

class HeartrateChar : public Atoll::PeerCharacteristicHeartrate {
    void onNotify(
        BLERemoteCharacteristic* c,
        uint8_t* data,
        size_t length,
        bool isNotify);
};

class HeartrateMonitor : public Atoll::HeartrateMonitor {
   public:
    HeartrateMonitor(
        const char* address,
        uint8_t addressType,
        const char* type,
        const char* name)
        : Atoll::HeartrateMonitor(
              address,
              addressType,
              type,
              name,
              new HeartrateChar()) {}
};

#endif