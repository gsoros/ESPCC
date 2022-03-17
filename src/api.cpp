#include "api.h"
#include "board.h"

void Api::setup() {
    Atoll::Api::setup();

    addCommand(ApiCommand("hostname", Api::hostnameProcessor));
    addCommand(ApiCommand("touchThres", Api::touchThresProcessor));
    addCommand(ApiCommand("touchRead", Api::touchReadProcessor));
    addCommand(ApiCommand("scan", Api::scanProcessor));
}

ApiResult *Api::touchThresProcessor(ApiReply *reply) {
    // arg format: padIndex:threshold
    // set touchpad threshold
    log_i("arg: %s", reply->arg);
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
        log_i("touchPad %d new threshold %d", index, thres);
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
            log_e("no space left for adding %s to %s", token, thresholds);
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
    log_i("index %d numPads %d", padIndex, board.touch.numPads);
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