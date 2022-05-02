#include "board.h"
#include "wifi.h"

void Wifi::applySettings() {
    Atoll::Wifi::applySettings();
    board.oled.onWifiStateChange();
};

void Wifi::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    Atoll::Wifi::onEvent(event, info);
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            board.oled.onWifiStateChange();
            break;
    }

    static bool prevConnected = 0;
    bool connected = board.wifi.isConnected();
    if (prevConnected != connected) {
        if (connected) {
            log_i("starting webserver");
            Serial.flush();
            board.webserver.begin();
        } else {
            board.webserver.end();
        }
    }
    prevConnected = connected;
}
