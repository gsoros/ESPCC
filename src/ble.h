#ifndef __ble_h
#define __ble_h

#include "definitions.h"
#include "atoll_ble.h"

class Ble : public Atoll::Ble {
   public:
    virtual void setup(const char *deviceName, ::Preferences *p) {
        Atoll::Ble::setup(deviceName, p);
    }

    void onWrite(BLECharacteristic *c);
};

#endif