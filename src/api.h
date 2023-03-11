#ifndef __api_h
#define __api_h

#include "definitions.h"
#include "atoll_api.h"
#include "ble_server.h"

class Api : public Atoll::Api {
   public:
    static void setup(Api *instance,
                      ::Preferences *p,
                      const char *preferencesNS,
                      BleServer *bleServer = nullptr,
                      const char *serviceUuid = nullptr);

   protected:
    static Api::Result *systemProcessor(Api::Message *);
    static Api::Result *touchProcessor(Api::Message *);

    static Api::Result *scanProcessor(Api::Message *);
    static Api::Result *scanResultProcessor(Api::Message *);
    static Api::Result *peersProcessor(Api::Message *);
    static Api::Result *addPeerProcessor(Api::Message *);
    static Api::Result *deletePeerProcessor(Api::Message *);

    static Api::Result *vescProcessor(Api::Message *);
};

#endif