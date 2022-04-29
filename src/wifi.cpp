#include "board.h"
#include "wifi.h"

void Wifi::applySettings() {
    Atoll::Wifi::applySettings();
    board.oled.onWifiStateChange();
};

void Wifi::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    Atoll::Wifi::onEvent(event, info);
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            board.oled.onWifiStateChange();
            board.webserver.begin();
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            board.oled.onWifiStateChange();
            break;
        default:
            break;
    }
}
