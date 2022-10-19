#include "board.h"
#include "ble_server.h"

#include "atoll_ble.h"

void BleServer::init() {
    /*
      0x00 BLE_HS_IO_DISPLAY_ONLY     DisplayOnly     IO capability
      0x01 BLE_HS_IO_DISPLAY_YESNO    DisplayYesNo    IO capability
      0x02 BLE_HS_IO_KEYBOARD_ONLY    KeyboardOnly    IO capability
      0x03 BLE_HS_IO_NO_INPUT_OUTPUT  NoInputNoOutput IO capability
      0x04 BLE_HS_IO_KEYBOARD_DISPLAY KeyboardDisplay IO capability
    */
    Atoll::Ble::init(deviceName, BLE_HS_IO_KEYBOARD_DISPLAY);
}

uint16_t BleServer::getAppearance() {
    return APPEARANCE_CYCLING_COMPUTER;
}

void BleServer::notifyApiTx(const char *str) {
    notify(BLEUUID(API_SERVICE_UUID),
           BLEUUID(API_TX_CHAR_UUID),
           (uint8_t *)str, strlen(str));
}