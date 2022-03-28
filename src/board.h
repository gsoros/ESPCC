#ifndef __board_h
#define __board_h

#include "definitions.h"
#include "atoll_preferences.h"
#include "atoll_task.h"
#ifdef FEATURE_SERIAL
#include "atoll_split_stream.h"
#include "atoll_wifi_serial.h"
#else
#include "atoll_null_serial.h"
#endif
#include "touch.h"
#include "ble_client.h"
#include "ble_server.h"
#include "atoll_gps.h"
#include "atoll_oled.h"
#include "atoll_sdcard.h"
#include "api.h"
#include "atoll_wifi.h"
#include "atoll_ota.h"
#include "atoll_battery.h"
#include "atoll_recorder.h"
#include "atoll_uploader.h"

class Board : public Atoll::Task,
              public Atoll::Preferences {
   public:
    const char *taskName() { return "Board"; }
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    ::Preferences arduinoPreferences = ::Preferences();
#ifdef FEATURE_SERIAL
    HardwareSerial hwSerial = HardwareSerial(0);
    Atoll::WifiSerial wifiSerial;
#endif
    Atoll::GPS gps;
    Atoll::Oled oled = Atoll::Oled(
        new U8G2_SH1106_128X64_NONAME_F_HW_I2C(
            U8G2_R1,        // rotation 90˚
            U8X8_PIN_NONE,  // reset pin none
            OLED_SCK_PIN,
            OLED_SDA_PIN));
    BleClient bleClient;
    BleServer bleServer;
    Atoll::SdCard sdcard = Atoll::SdCard(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    Touch touch = Touch(TOUCH_PAD_0_PIN);
    Api api;
    Atoll::Wifi wifi;
    Atoll::Ota ota;
    Atoll::Battery battery;
    Atoll::Recorder recorder;
    Atoll::Uploader uploader;

    Board() {}
    virtual ~Board() {}

    void setup() {
        setCpuFrequencyMhz(80);

#ifdef FEATURE_SERIAL
        hwSerial.begin(115200);
        Serial.setup(&hwSerial, &wifiSerial, true, true);
        while (!hwSerial) vTaskDelay(10);
        Serial.printf("\n\n\nESPCC %s %s\n\n\n", __DATE__, __TIME__);
#endif
        preferencesSetup(&arduinoPreferences, "BOARD");
        loadSettings();

        bleServer.setup(hostName);
        api.setup(&api, &arduinoPreferences, "API", &bleServer, API_SERVICE_UUID);
        gps.setup(9600, SWSERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN, &touch, &oled);
        oled.setup();
        bleClient.setup(hostName, &arduinoPreferences, &bleServer);
        sdcard.setup();
        // if (sdcard.mounted) sdcard.test();
        touch.setup(&arduinoPreferences, "Touch");
        wifi.setup(hostName, &arduinoPreferences, "Wifi", &wifi, &api, &ota, &recorder
#ifdef FEATURE_SERIAL
                   ,
                   &wifiSerial
#endif
        );
        battery.setup(&arduinoPreferences, BATTERY_PIN, &battery, &api, &bleServer);
        recorder.setup(&gps, &sdcard, &api, &recorder);
        uploader.setup(&recorder, &sdcard, &wifi);

        gps.taskStart(GPS_TASK_FREQ);
        bleClient.taskStart(BLE_CLIENT_TASK_FREQ);
        bleServer.taskStart(BLE_SERVER_TASK_FREQ);
        touch.taskStart(TOUCH_TASK_FREQ);
        oled.taskStart(OLED_TASK_FREQ);
        battery.taskStart(BATTERY_TASK_FREQ);
        recorder.taskStart(RECORDER_TASK_FREQ);
        uploader.taskStart(UPLOADER_TASK_FREQ);
        taskStart(BOARD_TASK_FREQ);

        bleServer.start();
        recorder.start();
    }

    void loop() {
        taskStop();
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