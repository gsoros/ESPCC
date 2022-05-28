#include "board.h"
#include "webserver.h"

void Webserver::start() {
    if (serving) {
        log_i("already serving");
        return;
    }
    if (!board.wifi.isEnabled()) {
        log_i("Wifi is disabled, not starting");
        return;
    }
    if (nullptr == server) {
        log_e("server is null");
        return;
    }
    server->begin();
    serving = true;
    log_i("serving on port %d", port);
}

void Webserver::stop() {
    if (!serving) {
        log_i("not serving");
        return;
    }
    if (nullptr == server) {
        log_e("server is null");
        return;
    }
    server->end();
    serving = false;
    log_i("stopped serving");
}