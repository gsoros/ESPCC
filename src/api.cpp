#include "api.h"
#include "board.h"

void Api::setup() {
    Atoll::Api::setup();
    addResult(ApiResult(7, "stringTooShort"));
    addCommand(ApiCommand(
        3,
        "touchSens",
        Api::touchSens));
}

ApiResult *Api::touchSens(const char *arg, char *reply, char *value) {
    ApiResult *r = success();
    // set touchpad sensitivity
    // TODO
    //
    // get touchpad sensitivities
    // reply already contains "commandcode=", we append our values
    char sensitivities[valueLength] = "";
    for (int i = 0; i < board.touch.numPads; i++) {
        char token[9];
        snprintf(token, sizeof(token), "%d:%d;", i, board.touch.pads[i].threshold);
        int16_t remaining = valueLength - strlen(sensitivities) - 1;
        if (remaining < strlen(token)) {
            log_i("could not add %s to %s", token, sensitivities);
            return result("stringTooShort");
        }
        strncat(sensitivities, token, remaining);
    }
    char replyTmp[replyLength];
    strncpy(replyTmp, reply, replyLength);
    snprintf(reply, replyLength, "%s%s", replyTmp, sensitivities);
    // set value
    strncpy(value, sensitivities, valueLength);
    return r;
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