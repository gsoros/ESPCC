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
    PowerMeter(Saved saved)
        : Atoll::PowerMeter(saved, new PowerChar(), new BattPMChar()) {}

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
    ESPM(Saved saved)
        : Atoll::ESPM(saved,
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
    HeartrateMonitor(Saved saved)
        : Atoll::HeartrateMonitor(saved, new HeartrateChar(), new BattHRMChar()) {}
    virtual void onDisconnect(BLEClient* client, int reason) override;
};

class Vesc : public Atoll::Vesc {
   public:
    Vesc(Atoll::Peer::Saved saved,
         Atoll::PeerCharacteristicVescRX* customVescRX = nullptr,
         Atoll::PeerCharacteristicVescTX* customVescTX = nullptr);
};

#endif