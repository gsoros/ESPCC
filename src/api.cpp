#include "api.h"
#include "board.h"

void Api::setup() {
    Atoll::Api::setup();

    addCommand(ApiCommand("hostname", Api::hostnameProcessor));
    addCommand(ApiCommand("touchThres", Api::touchThresProcessor));
    addCommand(ApiCommand("touchRead", Api::touchReadProcessor));
    addCommand(ApiCommand("scan", Api::scanProcessor));
    addCommand(ApiCommand("scanResult", Api::scanResultProcessor));
    addCommand(ApiCommand("peers", Api::peersProcessor));
    addCommand(ApiCommand("addPeer", Api::addPeerProcessor));
    addCommand(ApiCommand("deletePeer", Api::deletePeerProcessor));
}

ApiResult *Api::touchThresProcessor(ApiReply *reply) {
    // arg format: padIndex:threshold
    // set touchpad threshold
    if (reply->log) log_i("arg: %s", reply->arg);
    if (0 < strlen(reply->arg)) {
        char *colon = strstr(reply->arg, ":");
        if (!colon) return result("argInvalid");
        int argLen = strlen(reply->arg);
        if (argLen < 3) return result("argInvalid");
        int indexEnd = colon ? colon - reply->arg : argLen;
        if (indexEnd < 1) return result("argInvalid");
        char indexStr[3] = "";
        strncpy(indexStr, reply->arg, sizeof indexStr);
        int index = atoi(indexStr);
        if (index < 0 || board.touch.numPads - 1 < index) return result("argInvalid");
        int thresLen = argLen - indexEnd - 1;
        if (thresLen < 1) return result("argInvalid");
        char thresStr[4] = "";
        strncpy(thresStr, colon + 1, sizeof thresStr);
        int thres = atoi(thresStr);
        if (thres < 1 || 99 < thres) return result("argInvalid");
        if (reply->log) log_i("touchPad %d new threshold %d", index, thres);
        board.touch.setPadThreshold(index, thres);
        board.touch.saveSettings();
    }
    // get touchpad thresholds
    // value format: padIndex1:threshold1,padIndex2:threshold2...
    char thresholds[valueLength] = "";
    for (int i = 0; i < board.touch.numPads; i++) {
        char token[9];
        snprintf(
            token,
            sizeof(token),
            0 == i ? "%d:%d" : ",%d:%d",
            i,
            board.touch.pads[i].threshold);
        int16_t remaining = valueLength - strlen(thresholds) - 1;
        if (remaining < strlen(token)) {
            if (reply->log) log_e("no space left for adding %s to %s", token, thresholds);
            return result("internalError");
        }
        strncat(thresholds, token, remaining);
    }
    strncpy(reply->value, thresholds, valueLength);
    return success();
}

// get touchpad reading
// arg: padIndex
// reply format: padIndex:currentValue
ApiResult *Api::touchReadProcessor(ApiReply *reply) {
    if (strlen(reply->arg) < 1) return result("argInvalid");
    uint8_t padIndex = atoi(reply->arg);
    if (reply->log) log_i("index %d numPads %d", padIndex, board.touch.numPads);
    if (board.touch.numPads <= padIndex) return result("argInvalid");
    snprintf(reply->value, valueLength, "%d:%d", padIndex, board.touch.read(padIndex));
    return success();
}

ApiResult *Api::hostnameProcessor(ApiReply *reply) {
    // set hostname
    // TODO
    // get hostname
    strncpy(reply->value, board.hostName, valueLength);
    return success();
}

ApiResult *Api::scanProcessor(ApiReply *reply) {
    if (!strlen(reply->arg)) return result("argInvalid");
    uint32_t duration = atoi(reply->arg);
    if (duration < 1 || 120 < duration) return result("argInvalid");
    if (board.bleClient.scan->isScanning()) {
        snprintf(reply->value, sizeof(reply->value), "%s", "already scanning");
        return error();
    }
    board.bleClient.startScan(duration);
    snprintf(reply->value, sizeof(reply->value), "%d", duration);
    return success();
}

ApiResult *Api::scanResultProcessor(ApiReply *reply) {
    if (reply->log) log_e("command scanResult cannot be called directly, replies are generated after starting a scan");
    return error();
}

ApiResult *Api::peersProcessor(ApiReply *reply) {
    char value[valueLength] = "";
    int16_t remaining = 0;
    for (int i = 0; i < board.bleClient.peersMax; i++) {
        if (nullptr == board.bleClient.peers[i]) continue;
        if (board.bleClient.peers[i]->markedForRemoval) continue;
        char token[PeerDevice::packedMaxLength + 1];
        board.bleClient.peers[i]->pack(token, sizeof(token) - 1);
        strcat(token, ";");
        remaining = valueLength - strlen(value) - 1;
        if (remaining < strlen(token)) {
            if (reply->log) log_e("no space left for adding %s to %s", token, value);
            return result("internalError");
        }
        strncat(value, token, remaining);
    }
    strncpy(reply->value, value, valueLength);
    return success();
}

ApiResult *Api::addPeerProcessor(ApiReply *reply) {
    if (strlen(reply->arg) < sizeof(PeerDevice::address) + 3) {
        if (reply->log) log_e("arg too short (%d)", strlen(reply->arg));
        return result("argInvalid");
    }
    char address[sizeof(PeerDevice::address)] = "";
    char type[sizeof(PeerDevice::type)] = "";
    char name[sizeof(PeerDevice::name)] = "";
    if (!PeerDevice::unpack(
            reply->arg,
            address, sizeof(address),
            type, sizeof(type),
            name, sizeof(name))) {
        if (reply->log) log_e("could not unpack %s", reply->arg);
        return result("argInvalid");
    }
    if (board.bleClient.peerExists(address)) {
        if (reply->log) log_e("peer already exists: %s", reply->arg);
        return result("argInvalid");
    }
    PeerDevice *peer = board.bleClient.createPeer(address, type, name);
    if (nullptr == peer) {
        if (reply->log) log_e("could not create peer from %s", reply->arg);
        return error();
    }
    if (!board.bleClient.addPeer(peer)) {
        delete peer;
        if (reply->log) log_e("could not add peer from %s", reply->arg);
        return error();
    }
    board.bleClient.saveSettings();
    return success();
}

ApiResult *Api::deletePeerProcessor(ApiReply *reply) {
    if (strlen(reply->arg) < sizeof(PeerDevice::address) - 1) {
        if (reply->log) log_e("arg too short (%d)", strlen(reply->arg));
        return result("argInvalid");
    }
    log_i("calling bleClient.removePeer(%s)", reply->arg);
    uint8_t removed = board.bleClient.removePeer(reply->arg);
    if (0 < removed) {
        board.bleClient.saveSettings();
        snprintf(reply->value, sizeof(reply->value), "%d", removed);
        return success();
    }
    return error();
}