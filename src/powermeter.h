#ifndef __powermeter_h
#define __powermeter_h

#include "atoll_ble_peer_device.h"
#include "atoll_ble_peer_characteristic.h"

class PowerChar : public Atoll::BlePeerCharacteristicPower {
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
        const char* type,
        const char* name) : Atoll::PowerMeter(address,
                                              type,
                                              name) {
        // replace default char object
        PowerChar* powerChar = new PowerChar;
        log_i("deleted %d", deleteChars(powerChar->label));
        addChar(powerChar);
    }
};

#endif