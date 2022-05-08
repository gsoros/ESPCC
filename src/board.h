#ifndef __board_h
#define __board_h

#define DISPLAY_OLED
#include <Arduino.h>

#include "definitions.h"
#ifdef FEATURE_SERIAL
#include "atoll_split_stream.h"
#include "atoll_wifi_serial.h"
#else
#include "atoll_null_serial.h"
#endif
#include "atoll_preferences.h"
#include "atoll_task.h"
#include "atoll_time.h"

#include "touch.h"
#include "ble_client.h"
#include "ble_server.h"
#include "gps.h"
#ifdef DISPLAY_OLED
#include "oled.h"
#else
#include "lcd.h"
#endif
#include "atoll_sdcard.h"
#include "api.h"
#include "wifi.h"
#include "atoll_mdns.h"
#include "atoll_ota.h"
#include "battery.h"
#include "recorder.h"
#include "atoll_uploader.h"
#include "webserver.h"
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
#ifdef DISPLAY_OLED
    Oled display = Oled(
        new U8G2_SH1106_128X64_NONAME_F_HW_I2C(
            U8G2_R1,        // rotation 90˚
            U8X8_PIN_NONE,  // reset pin none
            OLED_SCK_PIN,
            OLED_SDA_PIN));
#else
    // Arduino_HWSPI lcdDataBus = Arduino_HWSPI(
    Arduino_ESP32SPI lcdDataBus = Arduino_ESP32SPI(
        LCD_A0_PIN,
        LCD_CS_PIN);
    Arduino_SSD1283A lcdDevice = Arduino_SSD1283A(
        &lcdDataBus,
        LCD_RST_PIN,
        2  // rotation: 180˚
    );
    Lcd display = Lcd(&lcdDevice);
#endif
    BleClient bleClient;
    BleServer bleServer;
    Atoll::SdCard sdcard = Atoll::SdCard(SD_SCK_PIN,
                                         SD_MISO_PIN,
                                         SD_MOSI_PIN,
                                         SD_CS_PIN);
    Touch touch = Touch(TOUCH_PAD_0_PIN,
                        TOUCH_PAD_1_PIN,
                        TOUCH_PAD_2_PIN,
                        TOUCH_PAD_3_PIN);
    Api api;
    Wifi wifi;
    Atoll::Ota ota;
    Atoll::Mdns mdns;
    Battery battery;
    Recorder recorder;
    // Atoll::Uploader uploader;
    Webserver webserver;

    Board() {}
    virtual ~Board() {}

    void setup() {
        setCpuFrequencyMhz(80);

#ifdef FEATURE_SERIAL
        hwSerial.begin(115200);
        wifiSerial.setup(hostName, 0, 0, WIFISERIAL_TASK_FREQ);
        Serial.setup(&hwSerial, &wifiSerial);
        while (!hwSerial) vTaskDelay(10);
#endif
        preferencesSetup(&arduinoPreferences, "BOARD");
        loadSettings();
        log_i("\n\n\n%s %s %s\n\n\n", hostName, __DATE__, __TIME__);
        log_i("Setting timezone %s", timezone);
        Atoll::setTimezone(timezone);

        bleServer.setup(hostName);
        api.setup(&api, &arduinoPreferences, "API", &bleServer, API_SERVICE_UUID);
        gps.setup(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        sdcard.setup();
#ifdef DISPLAY_OLED
        display.setup();  // oled
#else
        display.setup(LCD_SCK_PIN, LCD_MISO_PIN, LCD_SDA_PIN, LCD_CS_PIN, LCD_BACKLIGHT_PIN);
#endif
        bleClient.setup(hostName, &arduinoPreferences);
        touch.setup(&arduinoPreferences, "Touch");
        battery.setup(&arduinoPreferences, BATTERY_PIN, &battery, &api, &bleServer);
        recorder.setup(&gps, &sdcard, &api, &recorder);
        // uploader.setup(&recorder, &sdcard, &wifi);
        webserver.setup(&sdcard, &recorder, &ota);
        mdns.setup(hostName, 3232);
        wifi.setup(hostName, &arduinoPreferences, "Wifi", &wifi, &api, &ota, &recorder
#ifdef FEATURE_SERIAL
                   ,
                   &wifiSerial
#endif
        );

        bleServer.start();

        gps.taskStart(GPS_TASK_FREQ);
        bleClient.taskStart(BLE_CLIENT_TASK_FREQ, 8192);
        bleServer.taskStart(BLE_SERVER_TASK_FREQ);
        display.taskStart(DISPLAY_TASK_FREQ);
        battery.taskStart(BATTERY_TASK_FREQ);
        recorder.taskStart(RECORDER_TASK_FREQ);
        // uploader.taskStart(UPLOADER_TASK_FREQ);
        touch.taskStart(TOUCH_TASK_FREQ);
        //#ifdef FEATURE_SERIAL
        // wifiSerial.taskStart(WIFISERIAL_TASK_FREQ);
        //#endif
        taskStart(BOARD_TASK_FREQ, 8192);

        recorder.start();
    }

    void loop() {
        ulong t = millis();

        static ulong lastSync = 0;
        const uint syncFreq = 600000;  // every 10 minutes
        if ((lastSync + syncFreq < t) || !lastSync)
            if (gps.syncSystemTime())
                lastSync = t;

#ifdef FEATURE_SERIAL
        while (Serial.available()) {
            int i = Serial.read();
            if (0 <= i && i < UINT8_MAX) {
                Serial.write((uint8_t)i);           // echo serial input
                api.write((const uint8_t *)&i, 1);  // feed serial input to api
            }
        }
#endif
    }

    bool loadSettings() {
        if (!preferencesStartLoad()) return false;
        char tmpHostName[sizeof(hostName)];
        strncpy(tmpHostName,
                preferences->getString("hostName", hostName).c_str(),
                sizeof(hostName));
        if (1 < strlen(tmpHostName))
            strncpy(hostName,
                    tmpHostName,
                    sizeof(hostName));
        char tmpTz[sizeof(timezone)];
        strncpy(tmpTz,
                preferences->getString("tz", timezone).c_str(),
                sizeof(timezone));
        if (1 < strlen(tmpTz))
            strncpy(timezone, tmpTz, sizeof(timezone));
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