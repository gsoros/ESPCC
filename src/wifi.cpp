#include "wifi.h"
#include "board.h"

void Wifi::applySettings() {
    wifi_mode_t wifiMode = WiFi.getMode();

    Atoll::Wifi::applySettings();

    if (wifiMode == WiFi.getMode()) return;
    wifiMode = WiFi.getMode();
    if (wifiMode == WIFI_MODE_NULL) {
        board.ota.taskStop();
#ifdef FEATURE_SERIAL
        board.wifiSerial.taskStop();
#endif
    } else {
        board.ota.taskStop();
        board.ota.setup(board.hostName);
        board.ota.taskStart("ota");
#ifdef FEATURE_SERIAL
        board.wifiSerial.taskStop();
        board.wifiSerial.setup();
        board.wifiSerial.taskStart("wifiSerial");
#endif
    }
}

void Wifi::addCommands(Api *api) {
    if (nullptr == api) return;
    api->addCommand(ApiCommand("wifi", enabledProcessor));
    api->addCommand(ApiCommand("wifiAp", apProcessor));
    api->addCommand(ApiCommand("wifiApSSID", apSSIDProcessor));
    api->addCommand(ApiCommand("wifiApPassword", apPasswordProcessor));
    api->addCommand(ApiCommand("wifiSta", staProcessor));
    api->addCommand(ApiCommand("wifiStaSSID", staSSIDProcessor));
    api->addCommand(ApiCommand("wifiStaPassword", staPasswordProcessor));
}

ApiResult *Wifi::enabledProcessor(ApiReply *reply) {
    // set wifi
    bool newValue = true;  // enable wifi by default
    if (0 < strlen(reply->arg)) {
        if (0 == strcmp("false", reply->arg) || 0 == strcmp("0", reply->arg))
            newValue = false;
        board.wifi.setEnabled(newValue);
    }
    // get wifi
    strncpy(reply->value, board.wifi.isEnabled() ? "1" : "0", sizeof(reply->value));
    return Api::success();
}

ApiResult *Wifi::apProcessor(ApiReply *reply) {
    // set
    bool newValue = true;  // enable by default
    if (0 < strlen(reply->arg)) {
        if (0 == strcmp("false", reply->arg) ||
            0 == strcmp("0", reply->arg))
            newValue = false;
        board.wifi.settings.apEnabled = newValue;
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    // get
    strncpy(reply->value, board.wifi.settings.apEnabled ? "1" : "0",
            sizeof(reply->value));
    return Api::success();
}

ApiResult *Wifi::apSSIDProcessor(ApiReply *reply) {
    // set
    if (0 < strlen(reply->arg)) {
        strncpy(board.wifi.settings.apSSID, reply->arg,
                sizeof(board.wifi.settings.apSSID));
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    // get
    strncpy(reply->value, board.wifi.settings.apSSID,
            sizeof(reply->value));
    return Api::success();
}

ApiResult *Wifi::apPasswordProcessor(ApiReply *reply) {
    // set
    if (0 < strlen(reply->arg)) {
        strncpy(board.wifi.settings.apPassword, reply->arg,
                sizeof(board.wifi.settings.apPassword));
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    // get
    strncpy(reply->value, board.wifi.settings.apPassword,
            sizeof(reply->value));
    return Api::success();
}

ApiResult *Wifi::staProcessor(ApiReply *reply) {
    // set
    bool newValue = true;  // enable by default
    if (0 < strlen(reply->arg)) {
        if (0 == strcmp("false", reply->arg) ||
            0 == strcmp("0", reply->arg))
            newValue = false;
        board.wifi.settings.staEnabled = newValue;
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    // get
    strncpy(reply->value, board.wifi.settings.staEnabled ? "1" : "0",
            sizeof(reply->value));
    return Api::success();
}

ApiResult *Wifi::staSSIDProcessor(ApiReply *reply) {
    // set
    if (0 < strlen(reply->arg)) {
        strncpy(board.wifi.settings.staSSID, reply->arg,
                sizeof(board.wifi.settings.staSSID));
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    // get
    strncpy(reply->value, board.wifi.settings.staSSID,
            sizeof(reply->value));
    return Api::success();
}

ApiResult *Wifi::staPasswordProcessor(ApiReply *reply) {
    // set
    if (0 < strlen(reply->arg)) {
        strncpy(board.wifi.settings.staPassword, reply->arg,
                sizeof(board.wifi.settings.staPassword));
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    // get
    strncpy(reply->value, board.wifi.settings.staPassword,
            sizeof(reply->value));
    return Api::success();
}