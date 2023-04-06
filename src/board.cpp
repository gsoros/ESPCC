#include "board.h"

Board::Board() : arduinoPreferences(),
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
                     U8G2_R1,        // rotation 90˚
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
                             2),  // rotation: 180˚
                         130, 130, 3, 36, &spiMutex)
#endif
{
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    pinMode(LCD_CS_PIN, OUTPUT);
    digitalWrite(LCD_CS_PIN, HIGH);
}

Board::~Board() {
}

void Board::setup() {
    setCpuFrequencyMhz(80);

    esp_log_level_set("*", ESP_LOG_ERROR);
    Atoll::Log::setLevel(ESP_LOG_DEBUG);

#ifdef FEATURE_SERIAL
    hwSerial.begin(115200);
#ifdef FEATURE_WIFI_SERIAL
    wifiSerial.setup(hostName, 0, 0, WIFISERIAL_TASK_FREQ, 2048 + 1024);
    Serial.setup(&hwSerial, &wifiSerial);
#else
    Serial.setup(&hwSerial);
#endif
    while (!hwSerial) vTaskDelay(10);
#endif

    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    preferencesSetup(&arduinoPreferences, "BOARD");
    loadSettings();
    log_i("%s %s %s", hostName, __DATE__, __TIME__);
    // log_d("free heap: %d", xPortGetFreeHeapSize());
    // heap_caps_print_heap_info(MALLOC_CAP_DEFAULT | MALLOC_CAP_8BIT | MALLOC_CAP_32BIT);
    // log_d("timezone %s", timezone);
    Atoll::setTimezone(timezone);
    if (!otaMode) sdcard.setup();

    bleServer.setup(hostName);
    api.setup(&api, &arduinoPreferences, "API", &bleServer, API_SERVICE_UUID);
    if (!otaMode) gps.setup(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

#if DISPLAY_DEVICE == DISPLAY_OLED
    display.setup();  // oled
#elif DISPLAY_DEVICE == DISPLAY_LCD
    display.setup(LCD_BACKLIGHT_PIN);
#endif
    if (!otaMode) bleClient.setup(hostName, &arduinoPreferences);
    if (!otaMode) touch.setup(&arduinoPreferences, "Touch");
    battery.setup(&arduinoPreferences, BATTERY_PIN, &battery, &api, &bleServer);
    if (!otaMode) recorder.setup(&gps, &sdcard, &api, &recorder);
        // uploader.setup(&recorder, &sdcard, &wifi);
#ifdef FEATURE_WEBSERVER
    if (!otaMode) webserver.setup(&sdcard, &recorder, &ota);
#endif
    mdns.setup(hostName, 3232);
    wifi.setup(hostName, &arduinoPreferences, "Wifi", &wifi, &api, &ota);

    bleServer.start();

    if (otaMode) {
        // log_d("otaMode is true");
        saveOtaMode(false);
        ota.setup(hostName);
        bleServer.taskStart(BLE_SERVER_TASK_FREQ, 4096);
        display.taskStart(DISPLAY_TASK_FREQ, 4096 - 1024);
        battery.taskStart(BATTERY_TASK_FREQ, 4096 - 1024);
        // touch.taskStart(TOUCH_TASK_FREQ, 4096);
        taskStart(BOARD_TASK_FREQ, 4096 + 1024);
        wifi.start();
        wifi.setEnabled(true, false);
        display.onOta("waiting");
        return;
    }

    gps.taskStart(GPS_TASK_FREQ, 4096 - 1024);
    bleClient.taskStart(BLE_CLIENT_TASK_FREQ, 4096 - 1024);
    bleServer.taskStart(BLE_SERVER_TASK_FREQ, 4096);
    display.taskStart(DISPLAY_TASK_FREQ, 4096 - 1024);
    battery.taskStart(BATTERY_TASK_FREQ, 4096 - 1024);
    recorder.taskStart(RECORDER_TASK_FREQ, 4096 - 1024);
    // uploader.taskStart(UPLOADER_TASK_FREQ);
    touch.taskStart(TOUCH_TASK_FREQ, 4096);
    taskStart(BOARD_TASK_FREQ, 4096 + 2048);

    recorder.start();
    wifi.start();
}

void Board::loop() {
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

bool Board::loadSettings() {
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

    otaMode = preferences->getBool("ota", false);

    uint32_t tmpUint = PAS_MODE_PROPORTIONAL;
    tmpUint = preferences->getUInt("pasMode", tmpUint);
    if (PAS_MODE_MAX <= tmpUint) tmpUint = PAS_MODE_PROPORTIONAL;
    pasMode = tmpUint;

    tmpUint = 0;
    tmpUint = preferences->getUInt("pasLevel", tmpUint);
    if (1 < tmpUint) tmpUint = 1;  // always start with either 0 or 1
    pasLevel = tmpUint;

    tmpUint = preferences->getUInt("pasMHP", pasMinHumanPower);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    pasMinHumanPower = tmpUint;

    tmpUint = preferences->getUInt("pasCF", pasConstantFactor);
    if (UINT16_MAX < tmpUint) tmpUint = UINT16_MAX;
    pasConstantFactor = tmpUint;

    float tmpFloat = preferences->getFloat("pasPF", pasProportionalFactor);
    if (tmpFloat < 0.0f)
        tmpFloat = 0.0f;
    else if (20.0f < tmpFloat)
        tmpFloat = 20.0f;
    pasProportionalFactor = tmpFloat;

    tmpUint = preferences->getUInt("vescBNS", vescBattNumSeries);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    vescBattNumSeries = tmpUint;

    vescBattCapacityWh = preferences->getFloat("vescBC", vescBattCapacityWh);
    if (vescBattCapacityWh < 0.0f) vescBattCapacityWh = 0.0f;

    tmpUint = preferences->getUInt("vescMP", vescMaxPower);
    if (UINT16_MAX < tmpUint) tmpUint = UINT16_MAX;
    vescMaxPower = tmpUint;

    vescMinCurrent = preferences->getFloat("vescMiC", vescMinCurrent);
    if (vescMinCurrent < 0.0f) vescMinCurrent = 0.0f;

    vescMaxCurrent = preferences->getFloat("vescMaC", vescMaxCurrent);
    if (vescMaxCurrent < 0.0f) vescMaxCurrent = 0.0f;

    vescRampUp = preferences->getBool("vescRU", vescRampUp);

    vescRampDown = preferences->getBool("vescRD", vescRampDown);

    vescRampMinCurrentDiff = preferences->getFloat("vescRMCD", vescRampMinCurrentDiff);
    if (vescRampMinCurrentDiff < 0.0f) vescRampMinCurrentDiff = 0.0f;

    tmpUint = preferences->getUInt("vescRNS", vescRampNumSteps);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    vescRampNumSteps = tmpUint;

    tmpUint = preferences->getUInt("vescRT", vescRampTime);
    if (UINT16_MAX < tmpUint) tmpUint = UINT16_MAX;
    vescRampTime = tmpUint;

    tmpUint = preferences->getUInt("vescTMW", vescTMW);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    vescTMW = tmpUint;

    tmpUint = preferences->getUInt("vescTML", vescTML);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    vescTML = tmpUint;

    tmpUint = preferences->getUInt("vescTEW", vescTEW);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    vescTEW = tmpUint;

    tmpUint = preferences->getUInt("vescTEL", vescTEL);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    vescTEL = tmpUint;

    preferencesEnd();
    return true;
}

void Board::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putString("hostName", hostName);
    preferences->putString("tz", timezone);
    savePasSettings(true);
    saveVescSettings(true);
    preferencesEnd();
}

void Board::saveOtaMode(bool mode, bool skipStartEnd) {
    if (!skipStartEnd && !preferencesStartSave()) return;
    size_t written = preferences->putBool("ota", mode);
    log_d("mode: %d, written %d", mode, written);
    if (!skipStartEnd) preferencesEnd();
}

void Board::savePasSettings(bool skipStartEnd) {
    if (!skipStartEnd && !preferencesStartSave()) return;
    preferences->putUInt("pasMode", (uint32_t)pasMode);
    preferences->putUInt("pasLevel", (uint32_t)pasLevel);
    preferences->putUInt("pasMHP", (uint32_t)pasMinHumanPower);
    preferences->putUInt("pasCF", (uint32_t)pasConstantFactor);
    preferences->putFloat("pasPF", pasProportionalFactor);
    if (!skipStartEnd) preferencesEnd();
}

void Board::saveVescSettings(bool skipStartEnd) {
    if (!skipStartEnd && !preferencesStartSave()) return;
    preferences->putUInt("vescBNS", vescBattNumSeries);
    preferences->putFloat("vescBC", vescBattCapacityWh);
    preferences->putUInt("vescMP", vescMaxPower);
    preferences->putFloat("vescMiC", vescMinCurrent);
    preferences->putFloat("vescMaC", vescMaxCurrent);
    preferences->putBool("vescRU", vescRampUp);
    preferences->putBool("vescRD", vescRampDown);
    preferences->putFloat("vescRMCD", vescRampMinCurrentDiff);
    preferences->putUInt("vescRNS", vescRampNumSteps);
    preferences->putUInt("vescRT", vescRampTime);
    preferences->putUInt("vescTMW", vescTMW);
    preferences->putUInt("vescTML", vescTML);
    preferences->putUInt("vescTEW", vescTEW);
    preferences->putUInt("vescTEL", vescTEL);
    if (!skipStartEnd) preferencesEnd();
}
