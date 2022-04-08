#include "wifi.h"
#include "board.h"

void Wifi::onApConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Atoll::Wifi::onApConnected(event, info);
    board.oled.onWifiEvent();
    board.recWebserver.begin();
}
void Wifi::onApDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Atoll::Wifi::onApDisconnected(event, info);
    board.oled.onWifiEvent();
}
void Wifi::onStaConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Atoll::Wifi::onStaConnected(event, info);
    board.oled.onWifiEvent();
    board.recWebserver.begin();
}
void Wifi::onStaDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Atoll::Wifi::onStaDisconnected(event, info);
    board.oled.onWifiEvent();
}