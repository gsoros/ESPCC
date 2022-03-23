#ifndef __api_h
#define __api_h

#include "definitions.h"
#include "atoll_api.h"
#include "ble_server.h"

typedef Atoll::ApiResult ApiResult;
typedef Atoll::ApiReply ApiReply;

typedef ApiResult *(*ApiProcessor)(ApiReply *reply);

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
    static ApiResult *hostnameProcessor(ApiReply *reply);
    static ApiResult *touchThresProcessor(ApiReply *reply);
    static ApiResult *touchReadProcessor(ApiReply *reply);
    static ApiResult *scanProcessor(ApiReply *reply);
    static ApiResult *scanResultProcessor(ApiReply *reply);
    static ApiResult *peersProcessor(ApiReply *reply);
    static ApiResult *addPeerProcessor(ApiReply *reply);
    static ApiResult *deletePeerProcessor(ApiReply *reply);
};

#endif