#ifndef __ble_server_h
#define __ble_server_h

#include "definitions.h"
#include "atoll_api.h"

#define BLE_APPEARANCE APPEARANCE_CYCLING_COMPUTER

/*
0x00 BLE_HS_IO_DISPLAY_ONLY DisplayOnly IO capability
0x01 BLE_HS_IO_DISPLAY_YESNO DisplayYesNo IO capability
0x02 BLE_HS_IO_KEYBOARD_ONLY KeyboardOnly IO capability
0x03 BLE_HS_IO_NO_INPUT_OUTPUT NoInputNoOutput IO capability
0x04 BLE_HS_IO_KEYBOARD_DISPLAY KeyboardDisplay Only IO capability
*/
#define BLE_SECURITY_IOCAP BLE_HS_IO_KEYBOARD_DISPLAY

#include "atoll_ble_server.h"

class BleServer : public Atoll::BleServer {
   public:
    void notifyApiTx(const char *str);
};

#endif