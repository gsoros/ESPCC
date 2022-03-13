#ifndef __board_h
#define __board_h

#include "definitions.h"
#include "atoll_preferences.h"
#include "atoll_task.h"
#include "touch.h"
#include "ble.h"
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
    Ble ble;
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
        touch.setup();

        gps.taskStart("GPS Task", GPS_TASK_FREQ);
        ble.taskStart("BLE Task", BLE_TASK_FREQ);
        touch.taskStart("Touch Task", TOUCH_TASK_FREQ);
        taskStart("Board Task", BOARD_TASK_FREQ);

        if (sd.ready()) sd.test();
    }

    void loop() {
        static int16_t contrast = 0;
        static bool increase = true;
        if (contrast < 0) {
            contrast = 0;
            increase = true;
        } else if (UINT8_MAX <= contrast) {
            contrast = UINT8_MAX;
            increase = false;
        }
        // bool success =
        oled.setContrast((uint8_t)contrast);
        // Serial.printf("Contrast: %d %s\n", contrast, success ? "succ" : "fail");
        contrast += increase ? 50 : -50;
        /*
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