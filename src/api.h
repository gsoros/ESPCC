#ifndef __api_h
#define __api_h

#include "definitions.h"
#include "atoll_api.h"
#include "ble_server.h"

typedef Atoll::ApiResult ApiResult;
typedef Atoll::ApiMessage ApiMessage;

typedef ApiResult *(*ApiProcessor)(ApiMessage *reply);

class ApiCommand : public Atoll::ApiCommand {
   public:
    ApiCommand(
        const char *name = "",
        ApiProcessor processor = nullptr,
        uint8_t code = 0)
        : Atoll::ApiCommand(name, processor, code) {}
};

class Api : public Atoll::Api {
   public:
    static void setup(Api *instance,
                      ::Preferences *p,
                      const char *preferencesNS,
                      BleServer *bleServer = nullptr,
                      const char *serviceUuid = nullptr);

   protected:
    static ApiResult *systemProcessor(ApiMessage *);
    static ApiResult *touchProcessor(ApiMessage *);
    static ApiResult *touchThresProcessor(ApiMessage *);
    static ApiResult *touchReadProcessor(ApiMessage *);
    static ApiResult *scanProcessor(ApiMessage *);
    static ApiResult *scanResultProcessor(ApiMessage *);
    static ApiResult *peersProcessor(ApiMessage *);
    static ApiResult *addPeerProcessor(ApiMessage *);
    static ApiResult *deletePeerProcessor(ApiMessage *);
};

#endif