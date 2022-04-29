#include "board.h"
#include "webserver.h"

void Webserver::begin() {
    if (serving) {
        log_i("already serving");
        return;
    }
    if (!board.wifi.isEnabled()) {
        log_i("Wifi is disabled, not starting");
        return;
    }
    server->begin();
    serving = true;
    log_i("serving on port %d", port);
}

void Webserver::end() {
    if (!serving) {
        log_i("not serving");
        return;
    }
    server->end();
    serving = false;
    log_i("stopped serving");
}