#include "board.h"
#include "ble_server.h"

void BleServer::notifyApiTx(const char *str) {
    notify(BLEUUID(API_SERVICE_UUID),
           BLEUUID(API_TX_CHAR_UUID),
           (uint8_t *)str, strlen(str));
}