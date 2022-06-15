#include "board.h"
#include "api.h"

void Api::setup(
    Api *instance,
    ::Preferences *p,
    const char *preferencesNS,
    BleServer *bleServer,
    const char *serviceUuid) {
    Atoll::Api::setup(instance, p, preferencesNS, bleServer, serviceUuid);

    addCommand(ApiCommand("system", Api::systemProcessor));
    addCommand(ApiCommand("touch", Api::touchProcessor));
    addCommand(ApiCommand("touchThres", Api::touchThresProcessor));
    addCommand(ApiCommand("touchRead", Api::touchReadProcessor));
    addCommand(ApiCommand("scan", Api::scanProcessor));
    addCommand(ApiCommand("scanResult", Api::scanResultProcessor));
    addCommand(ApiCommand("peers", Api::peersProcessor));
    addCommand(ApiCommand("addPeer", Api::addPeerProcessor));
    addCommand(ApiCommand("deletePeer", Api::deletePeerProcessor));
}

ApiResult *Api::systemProcessor(ApiMessage *msg) {
    if (msg->argStartsWith("hostname")) {
        char buf[sizeof(board.hostName)] = "";
        msg->argGetParam("hostname:", buf, sizeof(buf));
        if (0 < strlen(buf)) {
            // set hostname
            if (strlen(buf) < 2) return result("argInvalid");
            if (!isAlNumStr(buf)) return result("argInvalid");
            strncpy(board.hostName, buf, sizeof(board.hostName));
            board.saveSettings();
        }
        // get hostname
        strncpy(msg->reply, board.hostName, msgReplyLength);
        return success();
    }

    else if (msg->argIs("ota") || msg->argIs("OTA")) {
        log_i("entering ota mode");
        log_i("free heap before: %d", xPortGetFreeHeapSize());
        log_i("stopping recorder");
        board.recorder.stop();
        board.recorder.taskStop();
        log_i("stopping touch task");
        board.touch.taskStop();
        log_i("stopping gps task");
        board.gps.taskStop();
        log_i("stopping bleClient");
        board.bleClient.stop();
        log_i("stopping webserver");
        board.webserver.stop();
        log_i("free heap after: %d", xPortGetFreeHeapSize());
        log_i("enabling wifi");
        board.otaMode = true;
        board.wifi.autoStartWebserver = false;
        board.wifi.setEnabled(true, false);
        board.display.onOta("waiting");
        msg->replyAppend("ota");
        return success();
    } else if (msg->argStartsWith("startWifiOnRecordingEnd")) {
        char buf[8] = "";
        msg->argGetParam("startWifiOnRecordingEnd:", buf, sizeof(buf));
        if (0 < strlen(buf)) {
            // set
            if (0 == strcmp(buf, "1") || 0 == strcmp(buf, "true"))
                board.wifi.startOnRecordingEnd = true;
            else if (0 == strcmp(buf, "0") || 0 == strcmp(buf, "false"))
                board.wifi.startOnRecordingEnd = false;
            else
                return argInvalid();
            board.wifi.saveSettings();
        }
        // get
        snprintf(msg->reply, sizeof(msg->reply), "%d", board.wifi.startOnRecordingEnd);
        return success();
    }
    msg->replyAppend("|", true);
    msg->replyAppend("hostname[:str]|ota|startWifiOnRecordingEnd[:0|1]");
    return Atoll::Api::systemProcessor(msg);
}

// get touchpad readings, get/set thresholds, en/disable touchpads or
// disable touchpads for a number of seconds
// arg: read|thresholds[:t0,...]|enabled[:0|1]|disableFor:numSeconds
// reply format: read:r0,...|thresholds:t0,...|enabled:0|1
ApiResult *Api::touchProcessor(ApiMessage *msg) {
    if (msg->log) log_i("arg: %s", msg->arg);

    if (msg->argIs("read")) {
        msg->replyAppend("read:");
        char buf[8];
        for (uint8_t i = 0; i < board.touch.numPads; i++) {
            if (0 < i) msg->replyAppend(",");
            snprintf(buf, sizeof(buf), "%d", board.touch.read(i));
            msg->replyAppend(buf);
        }
        return success();
    }

    if (0 == strlen(msg->arg) || msg->argStartsWith("thresholds")) {
        if (msg->argStartsWith("thresholds:")) {
            char buf[32];
            msg->argGetParam("thresholds:", buf, sizeof(buf));
            char str[4] = "";
            char *p = buf;
            uint8_t index = 0;
            uint8_t updated = 0;
            while (p < buf + strlen(buf)) {
                if ('0' <= *p && *p <= '9') {
                    if (sizeof(str) - 1 < strlen(str)) {
                        log_e("param too long");
                        return argInvalid();
                    }
                    strncat(str, p, 1);
                } else if (',' == *p || '\0' == *p) {
                    if (board.touch.numPads < index + 1) {
                        log_e("index: %d, numPads: %d", index, board.touch.numPads);
                        return argInvalid();
                    }
                    int thres = atoi(str);
                    char check[sizeof(int) * 8 + 1];
                    itoa(thres, check, 10);
                    snprintf(check, sizeof(check), "%d", thres);
                    if (0 != strcmp(str, check) || thres < 0 || UINT16_MAX < thres) {
                        log_e("str: %s, check: %s, thres: %d", str, check, thres);
                        return argInvalid();
                    }
                    if (board.touch.pads[index].threshold != thres) {
                        if (msg->log) log_i("setting pad %d thres %d", index, thres);
                        board.touch.pads[index].threshold = (uint16_t)thres;
                        updated++;
                    }
                    strncpy(str, "", sizeof(str));
                    index++;
                } else {
                    log_e("invalid char %c", *p);
                    return argInvalid();
                }
                p++;
            }
            if (0 < updated) board.touch.saveSettings();
        } else if (0 != strlen(msg->arg) && !msg->argIs("thresholds"))
            return argInvalid();
        msg->replyAppend("thresholds:");
        char buf[8];
        for (uint8_t i = 0; i < board.touch.numPads; i++) {
            if (0 < i) msg->replyAppend(",");
            snprintf(buf, sizeof(buf), "%d", board.touch.pads[i].threshold);
            msg->replyAppend(buf);
        }
        return success();
    }

    if (msg->argStartsWith("enabled")) {
        if (msg->argStartsWith("enabled:")) {
            char buf[2];
            msg->argGetParam("enabled:", buf, sizeof(buf));
            if (0 == strcmp(buf, "0"))
                board.touch.enabled = false;
            else if (0 == strcmp(buf, "1"))
                board.touch.enabled = true;
            else
                return argInvalid();
        } else if (!msg->argIs("enabled"))
            return argInvalid();
        snprintf(msg->reply, sizeof(msg->reply), "enabled:%d", (uint8_t)board.touch.enabled);
        return success();
    }

    if (msg->argStartsWith("disableFor:")) {
        char buf[4];
        msg->argGetParam("disableFor:", buf, sizeof(buf));
        int i = atoi(buf);
        char check[4];
        snprintf(check, sizeof(check), "%d", i);
        if (0 != strcmp(buf, check) || i < 1) return argInvalid();
        board.touch.enabled = false;
        board.touch.enableAfter = millis() + i * 1000;
        snprintf(msg->arg, sizeof(msg->arg), "disableFor:%d", i);
        return success();
    }

    msg->replyAppend("read|thresholds[:t0,...]|enabled[:0|1]|disableFor:numSeconds");
    return argInvalid();
}

ApiResult *Api::touchThresProcessor(ApiMessage *msg) {
    // arg format: padIndex:threshold
    // set touchpad threshold
    if (msg->log) log_i("arg: %s", msg->arg);
    if (0 < strlen(msg->arg)) {
        char *colon = strstr(msg->arg, ":");
        if (!colon) return result("argInvalid");
        int argLen = strlen(msg->arg);
        if (argLen < 3) return result("argInvalid");
        int indexEnd = colon ? colon - msg->arg : argLen;
        if (indexEnd < 1) return result("argInvalid");
        char indexStr[3] = "";
        strncpy(indexStr, msg->arg, sizeof indexStr);
        int index = atoi(indexStr);
        if (index < 0 || board.touch.numPads - 1 < index) return result("argInvalid");
        int thresLen = argLen - indexEnd - 1;
        if (thresLen < 1) return result("argInvalid");
        char thresStr[4] = "";
        strncpy(thresStr, colon + 1, sizeof thresStr);
        int thres = atoi(thresStr);
        if (thres < 1 || 99 < thres) return result("argInvalid");
        if (msg->log) log_i("touchPad %d new threshold %d", index, thres);
        board.touch.setPadThreshold(index, thres);
        board.touch.saveSettings();
    }
    // get touchpad thresholds
    // value format: padIndex1:threshold1,padIndex2:threshold2...
    char thresholds[msgReplyLength] = "";
    for (int i = 0; i < board.touch.numPads; i++) {
        char token[9];
        snprintf(
            token,
            sizeof(token),
            0 == i ? "%d:%d" : ",%d:%d",
            i,
            board.touch.pads[i].threshold);
        int16_t remaining = msgReplyLength - strlen(thresholds) - 1;
        if (remaining < strlen(token)) {
            if (msg->log) log_e("no space left for adding %s to %s", token, thresholds);
            return internalError();
        }
        strncat(thresholds, token, remaining);
    }
    strncpy(msg->reply, thresholds, msgReplyLength);
    return success();
}

// get touchpad reading or disable touchpad
// arg: padIndex|disableFor:numSeconds
// reply format: padIndex:currentValue[,padIndex:currentValue...]
ApiResult *Api::touchReadProcessor(ApiMessage *msg) {
    if (!strlen(msg->arg)) return result("argInvalid");
    char *disable = strstr(msg->arg, "disableFor:");
    if (nullptr != disable) {
        if (disable + strlen("disableFor:") < msg->arg + strlen(msg->arg)) {
            log_i("disableFor: %s", disable + strlen("disableFor:"));
            int secs = atoi(disable + strlen("disableFor:"));
            if (0 < secs && secs < UINT8_MAX) {
                if (msg->log) log_i("touch disabled for %ds", secs);
                board.touch.enabled = false;
                board.touch.enableAfter = millis() + secs * 1000;
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
            strncat(msg->reply, buf, msgReplyLength - strlen(msg->reply));
        }
        // log_i("val: %s", msg->value);
        return success();
    }
    uint8_t padIndex = (uint8_t)atoi(msg->arg);
    // if (msg->log) log_i("index %d numPads %d", padIndex, board.touch.numPads);
    if (board.touch.numPads <= padIndex) return result("argInvalid");
    snprintf(msg->reply, msgReplyLength, "%d:%d", padIndex, board.touch.read(padIndex));
    return success();
}

ApiResult *Api::scanProcessor(ApiMessage *msg) {
    if (!strlen(msg->arg)) return result("argInvalid");
    uint32_t duration = atoi(msg->arg);
    if (duration < 1 || 120 < duration) return result("argInvalid");
    if (board.bleClient.scan->isScanning()) {
        snprintf(msg->reply, sizeof(msg->reply), "%s", "already scanning");
        return error();
    }
    board.bleClient.startScan(duration);
    snprintf(msg->reply, sizeof(msg->reply), "%d", duration);
    return success();
}

ApiResult *Api::scanResultProcessor(ApiMessage *msg) {
    if (msg->log) log_e("command scanResult cannot be called directly, replies are generated after starting a scan");
    return error();
}

ApiResult *Api::peersProcessor(ApiMessage *msg) {
    char value[msgReplyLength] = "";
    int16_t remaining = 0;
    for (int i = 0; i < board.bleClient.peersMax; i++) {
        if (nullptr == board.bleClient.peers[i]) continue;
        if (board.bleClient.peers[i]->markedForRemoval) continue;
        char token[Peer::packedMaxLength + 1];
        board.bleClient.peers[i]->pack(token, sizeof(token) - 1);
        strcat(token, "|");
        remaining = msgReplyLength - strlen(value) - 1;
        if (remaining < strlen(token)) {
            if (msg->log) log_e("no space left for adding %s to %s", token, value);
            return internalError();
        }
        strncat(value, token, remaining);
    }
    strncpy(msg->reply, value, msgReplyLength);
    return success();
}

ApiResult *Api::addPeerProcessor(ApiMessage *msg) {
    if (strlen(msg->arg) < sizeof(Peer::address) + 5) {
        if (msg->log) log_e("arg too short (%d)", strlen(msg->arg));
        return result("argInvalid");
    }
    char address[sizeof(Peer::address)] = "";
    uint8_t addressType = 0;
    char type[sizeof(Peer::type)] = "";
    char name[sizeof(Peer::name)] = "";
    if (!Peer::unpack(
            msg->arg,
            address, sizeof(address),
            &addressType,
            type, sizeof(type),
            name, sizeof(name))) {
        if (msg->log) log_e("could not unpack %s", msg->arg);
        return result("argInvalid");
    }
    if (board.bleClient.peerExists(address)) {
        if (msg->log) log_e("peer already exists: %s", msg->arg);
        return result("argInvalid");
    }
    Peer *peer = board.bleClient.createPeer(address, addressType, type, name);
    if (nullptr == peer) {
        if (msg->log) log_e("could not create peer from %s", msg->arg);
        return error();
    }
    if (!board.bleClient.addPeer(peer)) {
        delete peer;
        if (msg->log) log_e("could not add peer from %s", msg->arg);
        return error();
    }
    board.bleClient.saveSettings();
    return success();
}

ApiResult *Api::deletePeerProcessor(ApiMessage *msg) {
    if (strlen(msg->arg) < sizeof(Peer::address) - 1) {
        if (msg->log) log_e("arg too short (%d)", strlen(msg->arg));
        return result("argInvalid");
    }
    log_i("calling bleClient.removePeer(%s)", msg->arg);
    uint8_t removed = board.bleClient.removePeer(msg->arg);
    if (0 < removed) {
        board.bleClient.saveSettings();
        snprintf(msg->reply, sizeof(msg->reply), "%d", removed);
        return success();
    }
    return error();
}
