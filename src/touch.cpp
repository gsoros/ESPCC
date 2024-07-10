#include "board.h"
#include "touch.h"

void Touch::fireEvent(uint8_t index, Event event) {
    Atoll::Touch::fireEvent(index, event);
    if (!board.display.onTouchEvent(&pads[index], event)) {
        return;  // not propagated
    }
    if (locked && Event::quintupleTouch != event) {
        log_i("locked, no action");
        return;  // must unlock
    }
    switch (event) {
        case Event::singleTouch: {
            if (TOUCH_PAD_TOPLEFT == index) {  // increase pas
                board.pasLevel++;
                if (board.pasMaxLevel < board.pasLevel) board.pasLevel = board.pasMaxLevel;
            } else if (TOUCH_PAD_BOTTOMLEFT == index) {  // decrease pas
                if (0 < board.pasLevel)
                    board.pasLevel--;
            }
            board.savePasSettings();
            log_d("pasLevel: %d", board.pasLevel);
            board.display.onPasChange();
        } break;
        case Event::longTouch: {
            if (TOUCH_PAD_TOPLEFT == index || TOUCH_PAD_BOTTOMLEFT == index) {
                board.pasMode = board.pasMode == PAS_MODE_PROPORTIONAL ? PAS_MODE_CONSTANT : PAS_MODE_PROPORTIONAL;
                board.savePasSettings();
                log_d("pasMode: %d", board.pasMode);
                board.display.onPasChange();
            } else if (TOUCH_PAD_TOPRIGHT == index) {  // tare
                board.bleClient.tarePowerMeter();
            } else if (TOUCH_PAD_BOTTOMRIGHT == index && board.recorder.start()) {  // start recording
                // disable wifi but don't save
                board.wifi.setEnabled(false, false);
            }
        } break;
        case Event::doubleTouch: {
            if (false && TOUCH_PAD_BOTTOMRIGHT == index && board.recorder.end()) {  // DISABLED
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
            if (TOUCH_PAD_TOPRIGHT != index) {
                log_i("need topright quintuple for lock");
            } else {
                locked = !locked;
                log_i("%slocked", locked ? "" : "un");
                board.display.onLockChanged(locked);
                char str[32] = "";
                snprintf(str, sizeof(str), "%d;%d=locked:%d", Api::success()->code, Api::command("touch")->code, locked ? 1 : 0);
                board.bleServer.notifyApiTx(str);
            }
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