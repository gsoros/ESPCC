#include "board.h"
#include "touch.h"

void Touch::fireEvent(uint8_t index, Event event) {
    Atoll::Touch::fireEvent(index, event);
    if (!board.display.onTouchEvent(&pads[index], event)) {
        return;  // not propagated
    }
    if (locked && event != Event::quintupleTouch) {
        log_i("locked, no action");
        return;  // must unlock
    }
    switch (event) {
        case Event::singleTouch: {
            if (0 == index) {  // top left to increase pas
                board.pasLevel++;
                if (12 < board.pasLevel) board.pasLevel = 12;
            } else if (2 == index) {  // bottom left to decrease pas
                if (0 < board.pasLevel)
                    board.pasLevel--;
            }
            board.savePasSettings();
            log_d("pasLevel: %d", board.pasLevel);
            board.display.onPasChange();
        } break;
        case Event::longTouch: {
            if (0 == index || 2 == index) {  // top left or bottom left
                board.pasMode = board.pasMode == PAS_MODE_PROPORTIONAL ? PAS_MODE_CONSTANT : PAS_MODE_PROPORTIONAL;
                board.savePasSettings();
                log_d("pasMode: %d", board.pasMode);
                board.display.onPasChange();
            } else if (1 == index) {  // top right
                board.bleClient.tarePowerMeter();
            } else if (3 == index && board.recorder.start()) {  // bottom right
                // disable wifi but don't save
                board.wifi.setEnabled(false, false);
            }
        } break;
        case Event::doubleTouch: {
            if (false && 3 == index && board.recorder.end()) {  // bottom right DISABLED
                if (board.wifi.startOnRecordingEnd) {
                    log_i("starting wifi");
#ifdef FEATURE_WEBSERVER
                    board.wifi.autoStartWebserver = true;
#endif
#ifdef FEATURE_SERIAL
                    board.wifi.autoStartWifiSerial = false;
#endif
                    // enable wifi but don't save
                    board.wifi.setEnabled(true, false);
                }
            }
        } break;
        case Event::quintupleTouch: {
            locked = !locked;
            log_i("%slocked", locked ? "" : "un");
            board.display.onLockChanged(locked);
            char str[32] = "";
            snprintf(str, sizeof(str), "%d;%d=locked:%d", Api::success()->code, Api::command("touch")->code, locked ? 1 : 0);
            board.bleServer.notifyApiTx(str);
        } break;
        default:
            break;
    }
}

void Touch::onEnabledChanged() {
    Atoll::Touch::onEnabledChanged();
    char str[32] = "";
    snprintf(str, sizeof(str), "%d;%d=enabled:%d", Api::success()->code, Api::command("touch")->code, enabled ? 1 : 0);
    board.bleServer.notifyApiTx(str);
}