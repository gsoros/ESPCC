#ifndef __api_h
#define __api_h

#include "definitions.h"
#include "atoll_api.h"

typedef Atoll::ApiCommand ApiCommand;
typedef Atoll::ApiResult ApiResult;

class Api : public Atoll::Api {
   public:
   protected:
    ApiResult *hostname(const char *arg, char *reply, char *value);
};

#endif