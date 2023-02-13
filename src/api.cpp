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

    // TODO move this to Atoll, peers=scan, peers=add:, etc.
    addCommand(ApiCommand("scan", Api::scanProcessor));
    addCommand(ApiCommand("scanResult", Api::scanResultProcessor));
    addCommand(ApiCommand("peers", Api::peersProcessor));
    addCommand(ApiCommand("addPeer", Api::addPeerProcessor));
    addCommand(ApiCommand("deletePeer", Api::deletePeerProcessor));

    addCommand(ApiCommand("vesc", Api::vescProcessor));
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
        /*
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
#ifdef FEATURE_WEBSERVER
        log_i("stopping webserver");
        board.webserver.stop();
#endif
        log_i("free heap after: %d", xPortGetFreeHeapSize());
        log_i("enabling wifi");
        board.otaMode = true;
#ifdef FEATURE_WEBSERVER
        board.wifi.autoStartWebserver = false;
#endif
        board.wifi.setEnabled(true, false);
        board.display.onOta("waiting");
        */
        log_i("saving ota mode");
        board.saveOtaMode(true);
        process("system=reboot");
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
            char *bufEnd = buf + strlen(buf);
            uint8_t updated = 0;
            char search[5];
            char *p;
            char valueS[4];
            for (uint8_t i = 0; i < board.touch.numPads; i++) {
                snprintf(search, sizeof(search), "%d:", i);
                p = strstr(buf, search);
                if (!p) continue;
                p += strlen(search);
                strncpy(valueS, "", sizeof(valueS));
                while (p < bufEnd) {
                    if (*p < '0' || '9' < *p)
                        break;
                    if (sizeof(valueS) - 1 < strlen(valueS)) {
                        log_e("param too long");
                        return argInvalid();
                    }
                    strncat(valueS, p, 1);
                    p++;
                }
                if (!strlen(valueS)) continue;
                int thres = atoi(valueS);
                char check[sizeof(int) * 8 + 1];
                itoa(thres, check, 10);
                snprintf(check, sizeof(check), "%d", thres);
                if (0 != strcmp(valueS, check) || thres < 0 || UINT16_MAX < thres) {
                    log_e("valueS: %s, check: %s, thres: %d", valueS, check, thres);
                    return argInvalid();
                }
                if (board.touch.pads[i].threshold != thres) {
                    if (msg->log) log_i("setting pad %d thres %d", i, thres);
                    board.touch.pads[i].threshold = (uint16_t)thres;
                    updated++;
                }
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

ApiResult *Api::scanProcessor(ApiMessage *msg) {
    if (!strlen(msg->arg)) return result("argInvalid");
    int duration = atoi(msg->arg);
    if (duration < 1 || 120 < duration) return result("argInvalid");
    if (board.bleClient.scan->isScanning()) {
        snprintf(msg->reply, sizeof(msg->reply), "%s", "already scanning");
        return error();
    }
    if (!board.bleClient.startScan(duration * 1000)) {  // convert duration from s to ms
        snprintf(msg->reply, sizeof(msg->reply), "%s", "could not start");
        return error();
    }
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
        board.bleClient.peers[i]->pack(token, sizeof(token) - 1, false);
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
    if (strlen(msg->arg) < sizeof(Peer::Saved::address) + 5) {
        if (msg->log) log_e("arg too short (%d)", strlen(msg->arg));
        return result("argInvalid");
    }
    Peer::Saved saved;
    if (!Peer::unpack(
            msg->arg,
            &saved)) {
        if (msg->log) log_e("could not unpack %s", msg->arg);
        return result("argInvalid");
    }
    if (board.bleClient.peerExists(saved.address)) {
        if (msg->log) log_e("peer already exists: %s", msg->arg);
        return result("argInvalid");
    }
    Peer *peer = board.bleClient.createPeer(saved);
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
    if (strlen(msg->arg) < sizeof(Peer::Saved::address) - 1) {
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

ApiResult *Api::vescProcessor(ApiMessage *msg) {
    if (msg->log) log_i("arg: %s", msg->arg);

    if (msg->argIs("")) {
        msg->replyAppend("battNumSeries[:i]|battCapacity[:f]|maxPower[:i]|minCurrent[:f]|maxCurrent[:f]|rampUp[:0|1]|rampDown[:0|1]|rampMinCurrentDiff[:f]|rampNumSteps[:i]|rampTime[:i]");
        return argInvalid();
    }

    if (msg->argStartsWith("battNumSeries")) {
        char buf[4] = "";
        if (msg->argGetParam("battNumSeries:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[4] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 1 || UINT8_MAX < i) return argInvalid();
            board.vescBattNumSeries = (uint8_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "battNumSeries:%d", board.vescBattNumSeries);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("battCapacity")) {
        char buf[8] = "";
        if (msg->argGetParam("battCapacity:", buf, sizeof(buf))) {
            float f = (float)atof(buf);
            if (f < 0.1f || 20000.0f < f) return argInvalid();
            board.vescBattCapacityWh = f;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "battCapacity:%.2f", board.vescBattCapacityWh);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("maxPower")) {
        char buf[8] = "";
        if (msg->argGetParam("maxPower:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[8] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT16_MAX < i) return argInvalid();
            board.vescMaxPower = (uint16_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "maxPower:%d", board.vescMaxPower);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("minCurrent")) {
        char buf[8] = "";
        if (msg->argGetParam("minCurrent:", buf, sizeof(buf))) {
            float f = (float)atof(buf);
            if (f < 0.0f || 100.0f < f) return argInvalid();
            board.vescMinCurrent = f;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "minCurrent:%.2f", board.vescMinCurrent);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("maxCurrent")) {
        char buf[8] = "";
        if (msg->argGetParam("maxCurrent:", buf, sizeof(buf))) {
            float f = (float)atof(buf);
            if (f < 0.0f || 200.0f < f) return argInvalid();
            board.vescMaxCurrent = f;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "maxCurrent:%.2f", board.vescMaxCurrent);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("rampUp")) {
        char buf[8] = "";
        if (msg->argGetParam("rampUp:", buf, sizeof(buf))) {
            if (0 == strcmp(buf, "1") || 0 == strcasecmp(buf, "true")) {
                board.vescRampUp = true;
                board.saveVescSettings();
            } else if (0 == strcmp(buf, "0") || 0 == strcasecmp(buf, "false")) {
                board.vescRampUp = false;
                board.saveVescSettings();
            }
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "rampUp:%d", board.vescRampUp);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("rampDown")) {
        char buf[8] = "";
        if (msg->argGetParam("rampDown:", buf, sizeof(buf))) {
            if (0 == strcmp(buf, "1") || 0 == strcasecmp(buf, "true")) {
                board.vescRampDown = true;
                board.saveVescSettings();
            } else if (0 == strcmp(buf, "0") || 0 == strcasecmp(buf, "false")) {
                board.vescRampDown = false;
                board.saveVescSettings();
            }
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "rampDown:%d", board.vescRampDown);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("rampMinCurrentDiff")) {
        char buf[8] = "";
        if (msg->argGetParam("rampMinCurrentDiff:", buf, sizeof(buf))) {
            float f = (float)atof(buf);
            if (f < 0.0f || 100.0f < f) return argInvalid();
            board.vescRampMinCurrentDiff = f;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "rampMinCurrentDiff:%.2f", board.vescRampMinCurrentDiff);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("rampNumSteps")) {
        char buf[4] = "";
        if (msg->argGetParam("rampNumSteps:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[4] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT8_MAX < i) return argInvalid();
            board.vescRampNumSteps = (uint8_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "rampNumSteps:%d", board.vescRampNumSteps);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("rampTime")) {
        char buf[8] = "";
        if (msg->argGetParam("rampTime:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[8] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT16_MAX < i) return argInvalid();
            board.vescRampTime = (uint16_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "rampTime:%d", board.vescRampTime);
        msg->replyAppend(reply);
        return success();
    }

    return argInvalid();
}