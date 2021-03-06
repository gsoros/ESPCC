#ifndef __board_h
#define __board_h

#include <Arduino.h>

#include <SPI.h>

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

#if DISPLAY_DEVICE == DISPLAY_OLED
#include "oled.h"
#elif DISPLAY_DEVICE == DISPLAY_LCD
#include "databus/Arduino_HWSPI.h"
//#include "databus/Arduino_ESP32SPI.h"
#include "display/Arduino_SSD1283A.h"
#include "lcd.h"
#else
#error Unsupported DISPLAY_DEVICE
#endif

#include "atoll_sdcard.h"
#include "api.h"
#include "wifi.h"
#include "atoll_mdns.h"
#include "ota.h"
#include "battery.h"
#include "recorder.h"
#include "atoll_uploader.h"
#ifdef FEATURE_WEBSERVER
#include "webserver.h"
#endif
#include "atoll_log.h"

class Board : public Atoll::Task,
              public Atoll::Preferences {
   public:
    const char *taskName() { return "Board"; }
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    char timezone[SETTINGS_STR_LENGTH] = TIMEZONE;
    SemaphoreHandle_t spiMutex = xSemaphoreCreateMutex();
    ::Preferences arduinoPreferences;
#ifdef FEATURE_SERIAL
    HardwareSerial hwSerial;
    Atoll::WifiSerial wifiSerial;
#endif
    GPS gps;
#if (DISPLAY_DEVICE == DISPLAY_OLED)
    Oled display;
#elif (DISPLAY_DEVICE == DISPLAY_LCD)
    Lcd display;
#else
#error unknown DISPLAY_DEVICE
#endif
    BleClient bleClient;
    BleServer bleServer;
    Atoll::SdCard sdcard;
    Touch touch;
    Api api;
    Wifi wifi;
    bool otaMode = false;
    Ota ota;
    Atoll::Mdns mdns;
    Battery battery;
    Recorder recorder;
    // Atoll::Uploader uploader;
#ifdef FEATURE_WEBSERVER
    Webserver webserver;
#endif

    Board() : arduinoPreferences(),
#ifdef FEATURE_SERIAL
              hwSerial(0),
#endif
              sdcard(SD_CS_PIN, &spiMutex),
              touch(TOUCH_PAD_0_PIN,
                    TOUCH_PAD_1_PIN,
                    TOUCH_PAD_2_PIN,
                    TOUCH_PAD_3_PIN),
#if (DISPLAY_DEVICE == DISPLAY_OLED)
              display(new U8G2_SH1106_128X64_NONAME_F_HW_I2C(
                  U8G2_R1,        // rotation 90??
                  U8X8_PIN_NONE,  // reset pin none
                  OLED_SCK_PIN,
                  OLED_SDA_PIN))
#elif (DISPLAY_DEVICE == DISPLAY_LCD)
              display(new Arduino_SSD1283A(
                          new Arduino_HWSPI(
                              LCD_A0_PIN,
                              LCD_CS_PIN,
                              SPI_SCK_PIN,
                              SPI_MOSI_PIN,
                              SPI_MISO_PIN,
                              &SPI,
                              true),  // shared interface
                          LCD_RST_PIN,
                          2),  // rotation: 180??
                      130, 130, 3, 36, &spiMutex)
#endif
    {
    }
    virtual ~Board() {}

    void setup() {
        setCpuFrequencyMhz(80);

#ifdef FEATURE_SERIAL
        hwSerial.begin(115200);
        wifiSerial.setup(hostName, 0, 0, WIFISERIAL_TASK_FREQ, 2048 + 1024);
        Serial.setup(&hwSerial, &wifiSerial);
        while (!hwSerial) vTaskDelay(10);
#endif
        preferencesSetup(&arduinoPreferences, "BOARD");
        loadSettings();
        log_i("\n\n\n%s %s %s\n\n\n", hostName, __DATE__, __TIME__);
        log_i("free heap: %d", xPortGetFreeHeapSize());
        // heap_caps_print_heap_info(MALLOC_CAP_DEFAULT | MALLOC_CAP_8BIT | MALLOC_CAP_32BIT);
        log_i("timezone %s", timezone);
        Atoll::setTimezone(timezone);

        bleServer.setup(hostName);
        api.setup(&api, &arduinoPreferences, "API", &bleServer, API_SERVICE_UUID);
        gps.setup(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
        sdcard.setup();
#if DISPLAY_DEVICE == DISPLAY_OLED
        display.setup();  // oled
#elif DISPLAY_DEVICE == DISPLAY_LCD
        display.setup(LCD_BACKLIGHT_PIN);
#endif
        bleClient.setup(hostName, &arduinoPreferences);
        touch.setup(&arduinoPreferences, "Touch");
        battery.setup(&arduinoPreferences, BATTERY_PIN, &battery, &api, &bleServer);
        recorder.setup(&gps, &sdcard, &api, &recorder);
// uploader.setup(&recorder, &sdcard, &wifi);
#ifdef FEATURE_WEBSERVER
        webserver.setup(&sdcard, &recorder, &ota);
#endif
        ota.setup(hostName);
        mdns.setup(hostName, 3232);
        wifi.setup(hostName, &arduinoPreferences, "Wifi", &wifi, &api, &ota);

        bleServer.start();

        gps.taskStart(GPS_TASK_FREQ, 4096 - 1024);
        bleClient.taskStart(BLE_CLIENT_TASK_FREQ, 4096 - 1024);
        bleServer.taskStart(BLE_SERVER_TASK_FREQ, 4096);
        display.taskStart(DISPLAY_TASK_FREQ, 4096 - 1024);
        battery.taskStart(BATTERY_TASK_FREQ, 4096 - 1024);
        recorder.taskStart(RECORDER_TASK_FREQ, 4096 - 1024);
        // uploader.taskStart(UPLOADER_TASK_FREQ);
        touch.taskStart(TOUCH_TASK_FREQ, 4096);
        taskStart(BOARD_TASK_FREQ, 4096 + 1024);

        recorder.start();
        wifi.start();
    }

    void loop() {
        ulong t = millis();

#ifdef FEATURE_SERIAL
        /*
        static ulong lastStatus = 0;
        const uint statusFreq = 5000;
        if ((lastStatus + statusFreq < t) || !lastStatus) {
            Serial.printf(
                "heap: %d stack: gps %d, bleC %d, bleS %d, disp %d, bat %d, rec %d, touch %d, ota %d, wifiS %d, board %d\n",
                xPortGetFreeHeapSize(),
                gps.taskGetLowestStackLevel(),
                bleClient.taskGetLowestStackLevel(),
                bleServer.taskGetLowestStackLevel(),
                display.taskGetLowestStackLevel(),
                battery.taskGetLowestStackLevel(),
                recorder.taskGetLowestStackLevel(),
                touch.taskGetLowestStackLevel(),
                ota.taskGetLowestStackLevel(),
                wifiSerial.taskGetLowestStackLevel(),
                taskGetLowestStackLevel());
            if (!touch.enabled)
                Serial.printf("touch disabled\n");
            // if (touch.taskRunning())
            Serial.printf("touch: %d %d %d %d\n",
                          touch.pads[0].last ? t - touch.pads[0].last : 0,
                          touch.pads[1].last ? t - touch.pads[1].last : 0,
                          touch.pads[2].last ? t - touch.pads[2].last : 0,
                          touch.pads[3].last ? t - touch.pads[3].last : 0);
            lastStatus = t;
        }
        */
        while (Serial.available()) {
            int i = Serial.read();
            if (0 <= i && i < UINT8_MAX) {
                Serial.write((uint8_t)i);           // echo serial input
                api.write((const uint8_t *)&i, 1);  // feed serial input to api
            }
        }
#endif

        static ulong lastSync = 0;
        const uint syncFreq = 600000;  // every 10 minutes
        if (gps.taskRunning() && ((lastSync + syncFreq < t) || !lastSync))
            if (gps.syncSystemTime())
                lastSync = t;
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