#ifndef __wifi_h
#define __wifi_h

#include "definitions.h"
#include "atoll_wifi.h"
#include "api.h"

class Wifi : public Atoll::Wifi {
   public:
    void setup(
        const char *hostName,
        ::Preferences *p,
        const char *preferencesNS,
        Api *api) {
        Atoll::Wifi::setup(hostName, p, preferencesNS);
        addCommands(api);
    }

    void applySettings();

    void addCommands(Api *api);
    static ApiResult *enabledProcessor(ApiReply *reply);
    static ApiResult *apProcessor(ApiReply *reply);
    static ApiResult *apSSIDProcessor(ApiReply *reply);
    static ApiResult *apPasswordProcessor(ApiReply *reply);
    static ApiResult *staProcessor(ApiReply *reply);
    static ApiResult *staSSIDProcessor(ApiReply *reply);
    static ApiResult *staPasswordProcessor(ApiReply *reply);

   protected:
};

#endif