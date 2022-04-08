#include "rec_webserver.h"
#include "board.h"

void RecWebserver::begin() {
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

void RecWebserver::end() {
    if (!serving) {
        log_i("not serving");
        return;
    }
    server->end();
    serving = false;
    log_i("stopped serving");
}