#include "board.h"
#include "wifi.h"

void Wifi::applySettings() {
    Atoll::Wifi::applySettings();
    board.display.onWifiStateChange();
};

void Wifi::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    Atoll::Wifi::onEvent(event, info);
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            board.display.onWifiStateChange();
            break;
    }

    static bool prevConnected = 0;
    bool connected = board.wifi.isConnected();
    if (prevConnected != connected) {
        if (connected) {
            if (board.otaMode) {
                log_i("restarting ota");
                board.ota.stop();
                board.ota.taskStop();
                board.ota.start();
                board.ota.taskStart();
            } else {
#ifdef FEATURE_SERIAL
                if (autoStartWifiSerial) {
                    log_i("restarting wifiSerial");
                    board.wifiSerial.stop();
                    board.wifiSerial.taskStop();
                    board.wifiSerial.start();
                    board.wifiSerial.taskStart();
                }
#endif
                if (autoStartWebserver) {
                    log_i("starting webserver");
                    board.webserver.start();
                }
            }
            log_i("starting mdns");
            board.mdns.start();
        } else {
#ifdef FEATURE_SERIAL
            log_i("stopping wifiSerial");
            board.wifiSerial.stop();
#endif
            log_i("stopping webserver");
            board.webserver.stop();
            log_i("stopping mdns");
            board.mdns.stop();
        }
    }
    prevConnected = connected;
}
