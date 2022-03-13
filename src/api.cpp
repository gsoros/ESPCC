#include "api.h"
#include "board.h"

void Api::setup() {
    Atoll::Api::setup();
    addResult(ApiResult(8, "stringTooShort"));
    addCommand(ApiCommand(
        3,
        "touchThres",
        Api::touchThres));
}

ApiResult *Api::touchThres(const char *arg, char *reply, char *value) {
    // arg format: padIndex:threshold
    // set touchpad threshold
    log_i("arg: %s", arg);
    if (0 < strlen(arg)) {
        char *colon = strstr(arg, ":");
        if (!colon) return result("argInvalid");
        int argLen = strlen(arg);
        if (argLen < 3) return result("argInvalid");
        int indexEnd = colon ? colon - arg : argLen;
        if (indexEnd < 1) return result("argInvalid");
        char indexStr[3] = "";
        strncpy(indexStr, arg, sizeof indexStr);
        int index = atoi(indexStr);
        if (index < 0 || board.touch.numPads - 1 < index) return result("argInvalid");
        int thresLen = argLen - indexEnd - 1;
        if (thresLen < 1) return result("argInvalid");
        char thresStr[4] = "";
        strncpy(thresStr, colon + 1, sizeof thresStr);
        int thres = atoi(thresStr);
        if (thres < 1 || 99 < thres) return result("argInvalid");
        log_i("touchPad %d new threshold %d", index, thres);
        // TODO
        board.touch.setPadThreshold(index, thres);
        board.touch.saveSettings();
    }
    // get touchpad thresholds
    // value format: pad1:threshold1;pad2:threshold2;...
    // reply already contains "commandcode=", we append our values
    char thresholds[valueLength] = "";
    for (int i = 0; i < board.touch.numPads; i++) {
        char token[9];
        snprintf(token, sizeof(token), "%d:%d;", i, board.touch.pads[i].threshold);
        int16_t remaining = valueLength - strlen(thresholds) - 1;
        if (remaining < strlen(token)) {
            log_i("could not add %s to %s", token, thresholds);
            return result("stringTooShort");
        }
        strncat(thresholds, token, remaining);
    }
    char replyTmp[replyLength];
    strncpy(replyTmp, reply, replyLength);
    snprintf(reply, replyLength, "%s%s", replyTmp, thresholds);
    // set value
    strncpy(value, thresholds, valueLength);
    return success();
}

ApiResult *Api::hostname(const char *arg, char *reply, char *value) {
    // set hostname
    // TODO
    // get hostname
    // reply already contains "commandcode=", we append our hostname
    char replyTmp[replyLength];
    strncpy(replyTmp, reply, replyLength);
    snprintf(reply, replyLength, "%s%s", replyTmp, board.hostName);
    // set value
    strncpy(value, board.hostName, valueLength);
    return success();
}