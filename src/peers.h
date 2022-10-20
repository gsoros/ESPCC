#ifndef __peers_h
#define __peers_h

#include "atoll_peer.h"
#include "atoll_peer_characteristic.h"

class BattPMChar : public Atoll::PeerCharacteristicBattery {
    virtual void notify() override;
};

class PowerChar : public Atoll::PeerCharacteristicPower {
    virtual void notify() override;
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
              new PowerChar(),
              new BattPMChar()) {}

    virtual void onDisconnect(BLEClient* client, int reason) override;
};

class ApiTxChar : public Atoll::PeerCharacteristicApiTX {
   public:
    virtual void notify() override;
};

class WeightChar : public Atoll::PeerCharacteristicWeightscale {
   public:
    virtual void notify() override;
};

class ESPM : public Atoll::ESPM {
   public:
    ESPM(const char* address,
         uint8_t addressType,
         const char* type,
         const char* name)
        : Atoll::ESPM(
              address,
              addressType,
              type,
              name,
              new PowerChar(),
              new BattPMChar(),
              new ApiTxChar,
              new Atoll::PeerCharacteristicApiRX(),
              new WeightChar()) {}

    virtual void onDisconnect(BLEClient* client, int reason) override;
};

class BattHRMChar : public Atoll::PeerCharacteristicBattery {
    virtual void notify() override;
};

class HeartrateChar : public Atoll::PeerCharacteristicHeartrate {
    virtual void notify() override;
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
              new HeartrateChar(),
              new BattHRMChar()) {}
    virtual void onDisconnect(BLEClient* client, int reason) override;
};

#endif