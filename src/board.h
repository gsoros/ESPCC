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
// #include "databus/Arduino_ESP32SPI.h"
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
    Ota ota;
    Atoll::Mdns mdns;
    Battery battery;
    Recorder recorder;
    // Atoll::Uploader uploader;
#ifdef FEATURE_WEBSERVER
    Webserver webserver;
#endif

    Board();
    virtual ~Board();
    void setup();
    void loop();
    bool loadSettings();
    void saveSettings();
    void saveOtaMode(bool mode, bool skipStartEnd = false);
    void savePasSettings(bool skipStartEnd = false);
    void saveVescSettings(bool skipStartEnd = false);

    bool otaMode = false;

    uint8_t pasMode = PAS_MODE_PROPORTIONAL;  //
    uint8_t pasLevel = 0;                     //
    uint8_t pasMinHumanPower = 50;            // W, threshold to activate pas
    uint16_t pasConstantFactor = 100;         // power factor in constant mode
    float pasProportionalFactor = 1.0f;       // power factor in proportional mode

    uint8_t vescBattNumSeries = 13;       // number of cells in series
    float vescBattCapacityWh = 740;       // 3.7V * 20Ah
    uint16_t vescMaxPower = 2500;         // power limit
    float vescMinCurrent = 1.1f;          // for startup
    float vescMaxCurrent = 50.0f;         // current limit
    bool vescRampUp = true;               //
    bool vescRampDown = true;             //
    float vescRampMinCurrentDiff = 1.0f;  //
    uint8_t vescRampNumSteps = 3;         //
    uint16_t vescRampTime = 500;          //
};

extern Board board;

#endif