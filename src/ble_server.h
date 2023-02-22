#ifndef __ble_server_h
#define __ble_server_h

#include "definitions.h"
#include "atoll_api.h"
#include "atoll_ble_server.h"

class BleServer : public Atoll::BleServer {
   public:
    virtual void init() override;
    virtual uint16_t getAppearance() override;
    
    void notifyApiTx(const char *str);
};

#endif