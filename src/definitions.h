#ifndef __definitions_h
#define __definitions_h

#define SETTINGS_STR_LENGTH 32              // maximum length of settings strings
#define BLE_CHAR_VALUE_MAXLENGTH 256        // maximum number of bytes written to ble characteristic values
;                                           //
;                                           // task frequencies in Hz
#define BOARD_TASK_FREQ 10                  //
#define BLE_TASK_FREQ 10                    //
#define GPS_TASK_FREQ 100                   //
#define TOUCH_TASK_FREQ 20                  //
#define OLED_TASK_FREQ 25                   //
;                                           //
#define SLEEP_DELAY_DEFAULT 15 * 60 * 1000  // 15m
#define SLEEP_DELAY_MIN 1 * 60 * 1000       // 1m
#define SLEEP_COUNTDOWN_AFTER 30 * 1000     // 30s countdown on the serial console
#define SLEEP_COUNTDOWN_EVERY 2000          // 2s
;                                           //
#define HOSTNAME "ESPCC"                    // default host name, used by ble and ota mdns
;                                           //
#define LED_PIN 22                          // onboard LED pin
#define BATTERY_PIN 35                      // pin for battery voltage measurement
#define GPS_RX_PIN 17                       //
#define GPS_TX_PIN 16                       //
;                                           //
;                                           //
#define TOUCH_NUM_PADS 1                    //
#define TOUCH_PAD_0_PIN 4                   //
#endif