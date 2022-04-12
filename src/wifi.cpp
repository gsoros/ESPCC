#include "wifi.h"
#include "board.h"

void Wifi::applySettings() {
    Atoll::Wifi::applySettings();
    board.oled.onWifiStateChange();
};

void Wifi::onApConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Atoll::Wifi::onApConnected(event, info);
    board.oled.onWifiStateChange();
    board.webserver.begin();
}
void Wifi::onApDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Atoll::Wifi::onApDisconnected(event, info);
    board.oled.onWifiStateChange();
}
void Wifi::onStaConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Atoll::Wifi::onStaConnected(event, info);
    board.oled.onWifiStateChange();
    board.webserver.begin();
}
void Wifi::onStaDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Atoll::Wifi::onStaDisconnected(event, info);
    board.oled.onWifiStateChange();
}