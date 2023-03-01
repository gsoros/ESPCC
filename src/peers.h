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
    ApiTxChar();
    virtual void notify() override;
    virtual void processApiReply(const char* reply);
    virtual void processInit(const char* value);
    virtual void onApiReply(uint8_t commandCode, const char* value, size_t len);
    char commands[ATOLL_API_MAX_COMMANDS][ATOLL_API_COMMAND_NAME_LENGTH];
    uint8_t commandCode(const char* name);
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
    virtual void loop() override;
    virtual void setPower(uint16_t power) override;
    virtual void onConnect(BLEClient* client) override;
    virtual void onDisconnect(BLEClient* client, int reason) override;
};

#endif