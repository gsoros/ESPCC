#ifndef __board_h
#define __board_h

#include "definitions.h"
#include "atoll_preferences.h"
#include "atoll_task.h"
#include "touch.h"
#include "ble_server.h"
#include "gps.h"
#include "oled.h"
#include "sdcard.h"
#include "api.h"

class Board : public Atoll::Task,
              public Atoll::Preferences {
   public:
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    ::Preferences arduinoPreferences = ::Preferences();
    GPS gps;
    Oled oled;
    BleServer ble;
    SdCard sd;
    Touch touch = Touch(TOUCH_PAD_0_PIN);
    Api api;

    Board() {
    }

    virtual ~Board() {}

    void setup() {
        setCpuFrequencyMhz(80);
        preferencesSetup(&arduinoPreferences, "BOARD");
        loadSettings();

        api.setup();
        gps.setup();
        oled.setup();
        ble.setup(hostName, &arduinoPreferences);
        sd.setup();
        touch.setup(&arduinoPreferences, "Touch");

        gps.taskStart("Gps Task", GPS_TASK_FREQ);
        ble.taskStart("BleServer Task", BLE_TASK_FREQ);
        touch.taskStart("Touch Task", TOUCH_TASK_FREQ);
        oled.taskStart("Oled Task", OLED_TASK_FREQ);
        taskStart("Board Task", BOARD_TASK_FREQ);

        if (sd.ready()) sd.test();
    }

    void loop() {
    }

    bool loadSettings() {
        if (!preferencesStartLoad()) return false;
        char tmpHostName[SETTINGS_STR_LENGTH];
        strncpy(tmpHostName, preferences->getString("hostName", hostName).c_str(), SETTINGS_STR_LENGTH);
        if (1 < strlen(tmpHostName)) {
            strncpy(hostName, tmpHostName, SETTINGS_STR_LENGTH);
        }
        preferencesEnd();
        return true;
    }

    void saveSettings() {
        if (!preferencesStartSave()) return;
        preferences->putString("hostName", hostName);
        preferencesEnd();
    }
};

extern Board board;

#endif