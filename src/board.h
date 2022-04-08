#ifndef __board_h
#define __board_h

#include <Arduino.h>

#include "definitions.h"
#include "atoll_preferences.h"
#include "atoll_task.h"
#include "atoll_time.h"
#ifdef FEATURE_SERIAL
#include "atoll_split_stream.h"
#include "atoll_wifi_serial.h"
#else
#include "atoll_null_serial.h"
#endif
#include "touch.h"
#include "ble_client.h"
#include "ble_server.h"
#include "gps.h"
#include "oled.h"
#include "atoll_sdcard.h"
#include "api.h"
#include "wifi.h"
#include "atoll_ota.h"
#include "battery.h"
#include "recorder.h"
#include "atoll_uploader.h"
#include "rec_webserver.h"
#include "atoll_log.h"

class Board : public Atoll::Task,
              public Atoll::Preferences {
   public:
    const char *taskName() { return "Board"; }
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    char timezone[SETTINGS_STR_LENGTH] = TIMEZONE;
    ::Preferences arduinoPreferences = ::Preferences();
#ifdef FEATURE_SERIAL
    HardwareSerial hwSerial = HardwareSerial(0);
    Atoll::WifiSerial wifiSerial;
#endif
    GPS gps;
    Oled oled = Oled(
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
    Wifi wifi;
    Atoll::Ota ota;
    Battery battery;
    Recorder recorder;
    // Atoll::Uploader uploader;
    RecWebserver recWebserver;

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
        log_i("Setting timezone %s", timezone);
        Atoll::setTimezone(timezone);

        bleServer.setup(hostName);
        api.setup(&api, &arduinoPreferences, "API", &bleServer, API_SERVICE_UUID);
        gps.setup(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        oled.setup();
        bleClient.setup(hostName, &arduinoPreferences, &bleServer);
        sdcard.setup();
        touch.setup(&arduinoPreferences, "Touch");
        wifi.setup(hostName, &arduinoPreferences, "Wifi", &wifi, &api, &ota, &recorder
#ifdef FEATURE_SERIAL
                   ,
                   &wifiSerial
#endif
        );
        battery.setup(&arduinoPreferences, BATTERY_PIN, &battery, &api, &bleServer);
        recorder.setup(&gps, &sdcard, &api, &recorder);
        // uploader.setup(&recorder, &sdcard, &wifi);
        recWebserver.setup(&sdcard, &recorder, &ota);

        gps.taskStart(GPS_TASK_FREQ);
        bleClient.taskStart(BLE_CLIENT_TASK_FREQ, 8192);
        bleServer.taskStart(BLE_SERVER_TASK_FREQ);
        oled.taskStart(OLED_TASK_FREQ);
        battery.taskStart(BATTERY_TASK_FREQ);
        recorder.taskStart(RECORDER_TASK_FREQ);
        // uploader.taskStart(UPLOADER_TASK_FREQ);
        touch.taskStart(TOUCH_TASK_FREQ);
        taskStart(BOARD_TASK_FREQ);

        bleServer.start();
        recorder.start();
    }

    void loop() {
        ulong t = millis();

        static ulong lastSync = 0;
        const uint syncFreq = 600000;  // every 10 minutes
        if (((lastSync < t - syncFreq) && (syncFreq < t)) || !lastSync)
            if (gps.syncSystemTime())
                lastSync = t;
#ifdef FEATURE_SERIAL
        while (wifiSerial.available()) Serial.write(wifiSerial.read());
#endif
    }

    bool loadSettings() {
        if (!preferencesStartLoad()) return false;
        char tmpHostName[sizeof(hostName)];
        strncpy(tmpHostName, preferences->getString("hostName", hostName).c_str(), sizeof(hostName));
        if (1 < strlen(tmpHostName)) {
            strncpy(hostName, tmpHostName, sizeof(hostName));
        }
        char tmpTz[sizeof(timezone)];
        strncpy(tmpTz, preferences->getString("tz", timezone).c_str(), sizeof(timezone));
        if (1 < strlen(tmpTz)) {
            strncpy(timezone, tmpTz, sizeof(timezone));
        }
        preferencesEnd();
        return true;
    }

    void saveSettings() {
        if (!preferencesStartSave()) return;
        preferences->putString("hostName", hostName);
        preferences->putString("tz", timezone);
        preferencesEnd();
    }
};

extern Board board;

#endif