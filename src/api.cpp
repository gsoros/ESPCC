#include "api.h"
#include "board.h"

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