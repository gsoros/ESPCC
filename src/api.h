#ifndef __api_h
#define __api_h

#include "definitions.h"
#include "atoll_api.h"

// typedef Atoll::ApiCommand ApiCommand;
typedef Atoll::ApiResult ApiResult;
typedef Atoll::ApiReply ApiReply;

typedef ApiResult *(*ApiProcessor)(ApiReply *reply);

class ApiCommand : public Atoll::ApiCommand {
   public:
    ApiCommand(
        uint8_t code = 0,
        const char *name = "",
        ApiProcessor processor = nullptr)
        : Atoll::ApiCommand(code, name, processor) {}
};

class Api : public Atoll::Api {
   public:
    static void setup();

   protected:
    static ApiResult *hostnameProcessor(ApiReply *reply);
    static ApiResult *touchThresProcessor(ApiReply *reply);
    static ApiResult *touchReadProcessor(ApiReply *reply);
};

#endif