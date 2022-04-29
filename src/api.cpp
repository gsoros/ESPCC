#include "board.h"
#include "api.h"

void Api::setup(
    Api *instance,
    ::Preferences *p,
    const char *preferencesNS,
    BleServer *bleServer,
    const char *serviceUuid) {
    Atoll::Api::setup(instance, p, preferencesNS, bleServer, serviceUuid);

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
    char thresholds[replyValueLength] = "";
    for (int i = 0; i < board.touch.numPads; i++) {
        char token[9];
        snprintf(
            token,
            sizeof(token),
            0 == i ? "%d:%d" : ",%d:%d",
            i,
            board.touch.pads[i].threshold);
        int16_t remaining = replyValueLength - strlen(thresholds) - 1;
        if (remaining < strlen(token)) {
            if (reply->log) log_e("no space left for adding %s to %s", token, thresholds);
            return result("internalError");
        }
        strncat(thresholds, token, remaining);
    }
    strncpy(reply->value, thresholds, replyValueLength);
    return success();
}

// get touchpad reading or disable touchpad
// arg: [padIndex|disableFor:intNumSeconds]
// reply format: padIndex:currentValue[,padIndex:currentValue...]
ApiResult *Api::touchReadProcessor(ApiReply *reply) {
    char *disable = strstr(reply->arg, "disableFor:");
    if (strlen(reply->arg) < 1 || nullptr != disable) {
        if (nullptr != disable) {
            if (disable + strlen("disableFor:") < reply->arg + strlen(reply->arg)) {
                // log_i("disable: %s", disable + strlen("disableFor:"));
                int secs = atoi(disable + strlen("disableFor:"));
                if (0 < secs && secs < UINT8_MAX) {
                    if (reply->log) log_i("touch disabled for %ds", secs);
                    board.touch.enabled = false;
                    board.touch.enableAfter = millis() + secs * 1000;
                }
            }
        }
        char buf[10] = "";
        for (uint8_t i = 0; i < board.touch.numPads; i++) {
            snprintf(buf,
                     sizeof(buf),
                     0 < i ? ",%d:%d" : "%d:%d",
                     i,
                     board.touch.read(i));
            // log_i("buf: %s", buf);
            strncat(reply->value, buf, replyValueLength - strlen(reply->value));
        }
        // log_i("val: %s", reply->value);
        return success();
    }
    uint8_t padIndex = (uint8_t)atoi(reply->arg);
    // if (reply->log) log_i("index %d numPads %d", padIndex, board.touch.numPads);
    if (board.touch.numPads <= padIndex) return result("argInvalid");
    snprintf(reply->value, replyValueLength, "%d:%d", padIndex, board.touch.read(padIndex));
    return success();
}

ApiResult *Api::hostnameProcessor(ApiReply *reply) {
    // set hostname
    // TODO
    // get hostname
    strncpy(reply->value, board.hostName, replyValueLength);
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
    char value[replyValueLength] = "";
    int16_t remaining = 0;
    for (int i = 0; i < board.bleClient.peersMax; i++) {
        if (nullptr == board.bleClient.peers[i]) continue;
        if (board.bleClient.peers[i]->markedForRemoval) continue;
        char token[Peer::packedMaxLength + 1];
        board.bleClient.peers[i]->pack(token, sizeof(token) - 1);
        strcat(token, "|");
        remaining = replyValueLength - strlen(value) - 1;
        if (remaining < strlen(token)) {
            if (reply->log) log_e("no space left for adding %s to %s", token, value);
            return result("internalError");
        }
        strncat(value, token, remaining);
    }
    strncpy(reply->value, value, replyValueLength);
    return success();
}

ApiResult *Api::addPeerProcessor(ApiReply *reply) {
    if (strlen(reply->arg) < sizeof(Peer::address) + 5) {
        if (reply->log) log_e("arg too short (%d)", strlen(reply->arg));
        return result("argInvalid");
    }
    char address[sizeof(Peer::address)] = "";
    uint8_t addressType = 0;
    char type[sizeof(Peer::type)] = "";
    char name[sizeof(Peer::name)] = "";
    if (!Peer::unpack(
            reply->arg,
            address, sizeof(address),
            &addressType,
            type, sizeof(type),
            name, sizeof(name))) {
        if (reply->log) log_e("could not unpack %s", reply->arg);
        return result("argInvalid");
    }
    if (board.bleClient.peerExists(address)) {
        if (reply->log) log_e("peer already exists: %s", reply->arg);
        return result("argInvalid");
    }
    Peer *peer = board.bleClient.createPeer(address, addressType, type, name);
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
    if (strlen(reply->arg) < sizeof(Peer::address) - 1) {
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
