#ifndef __wifi_h
#define __wifi_h

#include "definitions.h"
#include "atoll_wifi.h"

class Wifi : public Atoll::Wifi {
   public:
    virtual void applySettings() override;
    virtual void onApConnected(WiFiEvent_t event, WiFiEventInfo_t info) override;
    virtual void onApDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) override;
    virtual void onStaConnected(WiFiEvent_t event, WiFiEventInfo_t info) override;
    virtual void onStaDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) override;
};

#endif