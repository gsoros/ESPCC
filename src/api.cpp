#include "board.h"
#include "api.h"

void Api::setup(
    Api *instance,
    ::Preferences *p,
    const char *preferencesNS,
    BleServer *bleServer,
    const char *serviceUuid) {
    Atoll::Api::setup(instance, p, preferencesNS, bleServer, serviceUuid);

    addCommand(Api::Command("system", Api::systemProcessor));
    addCommand(Api::Command("touch", Api::touchProcessor));
    addCommand(Api::Command("vesc", Api::vescProcessor));
}

Api::Result *Api::systemProcessor(Api::Message *msg) {
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
Api::Result *Api::touchProcessor(Api::Message *msg) {
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

Api::Result *Api::vescProcessor(Api::Message *msg) {
    if (msg->log) log_d("arg: %s", msg->arg);

    if (msg->argIs("")) {
        char buf[100];
        snprintf(buf, sizeof(buf),
                 "BNS:%d|BC:%.2f|MP:%d|MiC:%.2f|MaC:%.2f|RU:%d|RD:%d",
                 board.vescSettings.battNumSeries,
                 board.vescSettings.battCapacityWh,
                 board.vescSettings.maxPower,
                 board.vescSettings.minCurrent,
                 board.vescSettings.maxCurrent,
                 board.vescSettings.rampUp,
                 board.vescSettings.rampDown);
        msg->replyAppend(buf);
        snprintf(buf, sizeof(buf),
                 "|RMCD:%.2f|RNS:%d|RT:%d|TMW:%d|TML:%d|TEW:%d|TEL:%d",
                 board.vescSettings.rampMinCurrentDiff,
                 board.vescSettings.rampNumSteps,
                 board.vescSettings.rampTime,
                 board.vescSettings.tMotorWarn,
                 board.vescSettings.tMotorLimit,
                 board.vescSettings.tEscWarn,
                 board.vescSettings.tEscLimit);
        msg->replyAppend(buf);
        return success();
    }

    if (msg->argStartsWith("BNS")) {
        char buf[4] = "";
        if (msg->argGetParam("BNS:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[4] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 1 || UINT8_MAX < i) return argInvalid();
            board.vescSettings.battNumSeries = (uint8_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "BNS:%d", board.vescSettings.battNumSeries);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("BC")) {
        char buf[8] = "";
        if (msg->argGetParam("BC:", buf, sizeof(buf))) {
            float f = (float)atof(buf);
            if (f < 0.1f || 20000.0f < f) return argInvalid();
            board.vescSettings.battCapacityWh = f;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "BC:%.2f", board.vescSettings.battCapacityWh);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("MP")) {
        char buf[8] = "";
        if (msg->argGetParam("MP:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[8] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT16_MAX < i) return argInvalid();
            board.vescSettings.maxPower = (uint16_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "MP:%d", board.vescSettings.maxPower);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("MiC")) {
        char buf[8] = "";
        if (msg->argGetParam("MiC:", buf, sizeof(buf))) {
            float f = (float)atof(buf);
            if (f < 0.0f || 100.0f < f) return argInvalid();
            board.vescSettings.minCurrent = f;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "MiC:%.2f", board.vescSettings.minCurrent);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("MaC")) {
        char buf[8] = "";
        if (msg->argGetParam("MaC:", buf, sizeof(buf))) {
            float f = (float)atof(buf);
            if (f < 0.0f || 200.0f < f) return argInvalid();
            board.vescSettings.maxCurrent = f;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "MaC:%.2f", board.vescSettings.maxCurrent);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("RU")) {
        char buf[8] = "";
        if (msg->argGetParam("RU:", buf, sizeof(buf))) {
            if (0 == strcmp(buf, "1") || 0 == strcasecmp(buf, "true")) {
                board.vescSettings.rampUp = true;
                board.saveVescSettings();
            } else if (0 == strcmp(buf, "0") || 0 == strcasecmp(buf, "false")) {
                board.vescSettings.rampUp = false;
                board.saveVescSettings();
            }
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "RU:%d", board.vescSettings.rampUp);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("RD")) {
        char buf[8] = "";
        if (msg->argGetParam("RD:", buf, sizeof(buf))) {
            if (0 == strcmp(buf, "1") || 0 == strcasecmp(buf, "true")) {
                board.vescSettings.rampDown = true;
                board.saveVescSettings();
            } else if (0 == strcmp(buf, "0") || 0 == strcasecmp(buf, "false")) {
                board.vescSettings.rampDown = false;
                board.saveVescSettings();
            }
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "RD:%d", board.vescSettings.rampDown);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("RMCD")) {
        char buf[8] = "";
        if (msg->argGetParam("RMCD:", buf, sizeof(buf))) {
            float f = (float)atof(buf);
            if (f < 0.0f || 100.0f < f) return argInvalid();
            board.vescSettings.rampMinCurrentDiff = f;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "RMCD:%.2f", board.vescSettings.rampMinCurrentDiff);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("RNS")) {
        char buf[4] = "";
        if (msg->argGetParam("RNS:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[4] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT8_MAX < i) return argInvalid();
            board.vescSettings.rampNumSteps = (uint8_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "RNS:%d", board.vescSettings.rampNumSteps);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("RT")) {
        char buf[8] = "";
        if (msg->argGetParam("RT:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[8] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT16_MAX < i) return argInvalid();
            board.vescSettings.rampTime = (uint16_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "RT:%d", board.vescSettings.rampTime);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("TMW")) {
        char buf[8] = "";
        if (msg->argGetParam("TMW:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[8] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT8_MAX < i) return argInvalid();
            board.vescSettings.tMotorWarn = (uint16_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "TMW:%d", board.vescSettings.tMotorWarn);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("TML")) {
        char buf[8] = "";
        if (msg->argGetParam("TML:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[8] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT8_MAX < i) return argInvalid();
            board.vescSettings.tMotorLimit = (uint16_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "TML:%d", board.vescSettings.tMotorLimit);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("TEW")) {
        char buf[8] = "";
        if (msg->argGetParam("TEW:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[8] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT8_MAX < i) return argInvalid();
            board.vescSettings.tEscWarn = (uint16_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "TEW:%d", board.vescSettings.tEscWarn);
        msg->replyAppend(reply);
        return success();
    }

    if (msg->argStartsWith("TEL")) {
        char buf[8] = "";
        if (msg->argGetParam("TEL:", buf, sizeof(buf))) {
            int i = atoi(buf);
            char check[8] = "";
            snprintf(check, sizeof(check), "%d", i);
            if (0 != strcmp(buf, check) || i < 0 || UINT8_MAX < i) return argInvalid();
            board.vescSettings.tEscLimit = (uint16_t)i;
            board.saveVescSettings();
        }
        char reply[32];
        snprintf(reply, sizeof(reply), "TEL:%d", board.vescSettings.tEscLimit);
        msg->replyAppend(reply);
        return success();
    }

    return argInvalid();
}