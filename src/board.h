#ifndef __board_h
#define __board_h

#include "Arduino.h"

#include "definitions.h"
#include "atoll_preferences.h"
#include "atoll_task.h"
#include "atoll_touch.h"
#include "ble.h"
#include "gps.h"
#include "oled.h"
#include "sdcard.h"

class Board : public Atoll::Task,
              public Atoll::Preferences {
   public:
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    ::Preferences arduinoPreferences = ::Preferences();
    GPS gps;
    Oled oled;
    Ble ble;
    SdCard sd;
    Atoll::Touch touch = Atoll::Touch(ATOLL_TOUCH_PAD_0);

    Board() {
    }

    virtual ~Board() {}

    void setup() {
        setCpuFrequencyMhz(80);
        preferencesSetup(&arduinoPreferences, "BOARD");
        loadSettings();

        gps.setup();
        oled.setup();
        ble.setup("devicename", &arduinoPreferences);
        sd.setup();
        touch.setup();

        gps.taskStart("GPS Task", GPS_TASK_FREQ);
        ble.taskStart("BLE Task", BLE_TASK_FREQ);
        touch.taskStart("Touch Task", TOUCH_TASK_FREQ);
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