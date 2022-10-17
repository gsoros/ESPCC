#ifndef __ble_server_h
#define __ble_server_h

#include "definitions.h"
#include "atoll_api.h"

#define BLE_APPEARANCE APPEARANCE_CYCLING_COMPUTER

#include "atoll_ble_server.h"

class BleServer : public Atoll::BleServer {
   public:
    void notifyApiTx(const char *str) {
        notify(BLEUUID(API_SERVICE_UUID),
               BLEUUID(API_TX_CHAR_UUID),
               (uint8_t *)str, strlen(str));
    }
};

#endif