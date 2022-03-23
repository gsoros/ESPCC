#ifndef __board_h
#define __board_h

#include "definitions.h"
#include "atoll_preferences.h"
#include "atoll_task.h"
#include "touch.h"
#include "ble_client.h"
#include "ble_server.h"
#include "gps.h"
#include "oled.h"
#include "sdcard.h"
#include "api.h"
#include "atoll_wifi.h"
#include "atoll_ota.h"
#include "atoll_battery.h"

class Board : public Atoll::Task,
              public Atoll::Preferences {
   public:
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    ::Preferences arduinoPreferences = ::Preferences();
    GPS gps;
    Oled oled;
    BleClient bleClient;
    BleServer bleServer;
    SdCard sd;
    Touch touch = Touch(TOUCH_PAD_0_PIN);
    Api api;
    Atoll::Wifi wifi;
    Atoll::Ota ota;
    Atoll::Battery battery;

    Board() {}
    virtual ~Board() {}

    void setup() {
        setCpuFrequencyMhz(80);
        preferencesSetup(&arduinoPreferences, "BOARD");
        loadSettings();

        bleServer.setup(hostName);
        api.setup(&api, &arduinoPreferences, "API", &bleServer, API_SERVICE_UUID);
        gps.setup();
        oled.setup();
        bleClient.setup(hostName, &arduinoPreferences, &bleServer);
        sd.setup();
        touch.setup(&arduinoPreferences, "Touch");
        wifi.setup(hostName, &arduinoPreferences, "Wifi", &wifi, &api, &ota);
        battery.setup(&arduinoPreferences, &battery, &api, &bleServer);

        gps.taskStart("Gps Task", GPS_TASK_FREQ);
        bleClient.taskStart("BleClient Task", BLE_CLIENT_TASK_FREQ);
        bleServer.taskStart("BleServer Task", BLE_SERVER_TASK_FREQ);
        touch.taskStart("Touch Task", TOUCH_TASK_FREQ);
        oled.taskStart("Oled Task", OLED_TASK_FREQ);
        battery.taskStart("Battery Task", BATTERY_TASK_FREQ);
        taskStart("Board Task", BOARD_TASK_FREQ);
        bleServer.start();

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