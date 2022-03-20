#include "ble_server.h"
#include "board.h"

void BleServer::onWrite(BLECharacteristic *c) {
    if (c->getHandle() == apiRxChar->getHandle()) {
        ApiReply reply = board.api.process(c->getValue().c_str());

        // length = length(uint8max) + ":" + resultName
        char resultStr[4 + ATOLL_API_RESULT_NAME_LENGTH];

        if (reply.result->code == Api::success()->code) {
            // in case of success we omit the resultName: "resultCode;..."
            snprintf(resultStr, sizeof(resultStr), "%d", reply.result->code);
        } else {
            // in case of an error we provide the resultName: "resultCode:resultName;..."
            snprintf(resultStr, sizeof(resultStr), "%d:%s",
                     reply.result->code, reply.result->name);
        }
        char croppedReply[BLE_CHAR_VALUE_MAXLENGTH] = "";
        if (reply.result->code == Api::success()->code) {
            // in case of success we append the value
            snprintf(croppedReply, sizeof(croppedReply), "%s;%d=%s",
                     resultStr, reply.commandCode, reply.value);
        } else {
            // in case of an error we append the arg
            snprintf(croppedReply, sizeof(croppedReply), "%s;%d=%s",
                     resultStr, reply.commandCode, reply.arg);
        }
        log_i("apiRxChar reply: %s", croppedReply);
        apiTxChar->setValue(croppedReply);
        apiTxChar->notify();
    }
    Atoll::BleServer::onWrite(c);
};