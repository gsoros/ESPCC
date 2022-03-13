#include "ble.h"
#include "board.h"

void Ble::onWrite(BLECharacteristic *c) {
    if (c->getHandle() == apiRxChar->getHandle()) {
        char reply[board.api.replyLength] = "";
        char value[board.api.valueLength] = "";
        // Atoll::ApiResult *result =
        board.api.process(c->getValue().c_str(), reply, value);
        char croppedReply[strlen(reply) + 1];
        strncpy(croppedReply, reply, sizeof croppedReply);
        apiTxChar->setValue(croppedReply);
        apiTxChar->notify();
    }
    Atoll::Ble::onWrite(c);
};