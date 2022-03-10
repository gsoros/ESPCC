#ifndef __board_h
#define __board_h

#include "definitions.h"
#include "haspreferences.h"
#include "task.h"
#include "gps.h"
#include "oled.h"
#include "ble.h"
#include "sdcard.h"

class Board : public Task, public HasPreferences {
   public:
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    Preferences boardPreferences = Preferences();
    GPS gps;
    Oled oled;
    BLE ble;
    SdCard sd;

    Board() {}
    virtual ~Board() {}

    void setup() {
        setCpuFrequencyMhz(80);
        preferencesSetup(&boardPreferences, "BOARD");
        loadSettings();

        gps.setup();
        oled.setup();
        ble.setup("devicename", preferences);
        sd.setup();

        gps.taskStart("GPS Task", GPS_TASK_FREQ);
        ble.taskStart("BLE Task", BLE_TASK_FREQ);
        taskStart("Board Task", BOARD_TASK_FREQ);

        if (sd.ready()) sd.test();
    }

    void loop() {
        /*
        static uint8_t contrast = 0;
        static bool increase = true;
        Serial.printf("Contrast: %d\n", contrast);
        oled.setContrast(contrast);
        contrast += increase ? 1 : -1;
        if (0 == contrast || UINT8_MAX == contrast) increase = !increase;
        while (!Serial.available())
            vTaskDelay(10);
        Serial.read();
        */
        vTaskDelay(100000);
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