#include "ble_server.h"
#include "board.h"

void BleServer::onWrite(BLECharacteristic *c) {
    if (c->getHandle() == apiRxChar->getHandle()) {
        ApiReply reply = board.api.process(c->getValue().c_str());
        char resultStr[4 + ATOLL_API_RESULT_NAME_LENGTH];  // uint8 + ":" + resultName
        // in case of success we omit the resultName: "resultCode;..."
        if (reply.result->code == Api::success()->code)
            snprintf(resultStr, sizeof(resultStr), "%d", reply.result->code);
        else  // in case of an error we provide the resultName: "resultCode:resultName;..."
            snprintf(resultStr, sizeof(resultStr), "%d:%s",
                     reply.result->code, reply.result->name);
        char croppedReply[BLE_CHAR_VALUE_MAXLENGTH] = "";
        // in case of success we provide the value
        if (reply.result->code == Api::success()->code)
            snprintf(croppedReply, sizeof(croppedReply), "%s;%d=%s",
                     resultStr, reply.commandCode, reply.value);
        else
            snprintf(croppedReply, sizeof(croppedReply), "%s;%d",
                     resultStr, reply.commandCode);
        log_i("reply: %s", croppedReply);
        apiTxChar->setValue(croppedReply);
        apiTxChar->notify();
    }
    Atoll::BleServer::onWrite(c);
};