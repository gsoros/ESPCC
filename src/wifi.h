#ifndef __wifi_h
#define __wifi_h

#include "definitions.h"
#include "atoll_wifi.h"

class Wifi : public Atoll::Wifi {
   public:
    bool autoStartWebserver = true;
#ifdef FEATURE_SERIAL
    bool autoStartWifiSerial = true;
#endif
    bool startOnRecordingEnd = false;

    virtual void loadSettings() override;
    virtual void saveSettings() override;
    virtual void applySettings() override;
    virtual void onEvent(arduino_event_id_t event, arduino_event_info_t info) override;
};

#endif