#ifndef __definitions_h
#define __definitions_h

#define SETTINGS_STR_LENGTH 32                 // maximum length of settings strings
#define BLE_CHAR_VALUE_MAXLENGTH 512           // maximum number of bytes written to ble characteristic values
;                                              //
;                                              // task frequencies in Hz
#ifdef FEATURE_SERIAL                          //
#define BOARD_TASK_FREQ 2.0f                   //
#else                                          //
#define BOARD_TASK_FREQ 0.1f                   //
#endif                                         //
#define BLE_CLIENT_TASK_FREQ 0.5f              //
#define BLE_SERVER_TASK_FREQ 1.0f              //
#define GPS_TASK_FREQ 3.0f                     //
#define TOUCH_TASK_FREQ 20.0f                  //
#define DISPLAY_TASK_FREQ 1.0f                 //
#define OTA_TASK_FREQ 1.0f                     //
#define BATTERY_TASK_FREQ 1.0f                 //
#define WIFISERIAL_TASK_FREQ 2.0f              //
#define RECORDER_TASK_FREQ 1.0f                //
#define UPLOADER_TASK_FREQ 0.1f                // once every 10 secs
;                                              //
#define SLEEP_DELAY_DEFAULT 15 * 60 * 1000     // 15m
#define SLEEP_DELAY_MIN 1 * 60 * 1000          // 1m
#define SLEEP_COUNTDOWN_AFTER 30 * 1000        // 30s countdown on the serial console
#define SLEEP_COUNTDOWN_EVERY 2000             // 2s
;                                              //
#ifdef HOSTNAME                                //
#undef HOSTNAME                                //
#endif                                         //
#define HOSTNAME "ESPCC"                       // default host name
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"  //
;                                              //
#define DISPLAY_OLED 0                         //
#define DISPLAY_LCD 1                          //
#define DISPLAY_DEVICE DISPLAY_LCD             //
#define DISPLAY_NUM_PAGES 4                    //
#define DISPLAY_OLED_NUM_FIELDS 3              //
#define DISPLAY_LCD_NUM_FIELDS 3               //
#define DISPLAY_NUM_FEEDBACK 4                 //
;                                              //
#define PAS_MODE_PROPORTIONAL 0                //
#define PAS_MODE_CONSTANT 1                    //
#define PAS_MODE_MAX 2                         //
;                                              //
#define LED_PIN 22                             // onboard LED pin
#define BATTERY_PIN 35                         // pin for battery voltage measurement
#define GPS_RX_PIN 17                          // -> T
#define GPS_TX_PIN 16                          // -> R
;                                              //
#define SPI_SCK_PIN 18                         // spi clock
#define SPI_MISO_PIN 19                        // spi miso
#define SPI_MOSI_PIN 23                        // spi mosi, lcd sda
;                                              //
#define OLED_SCK_PIN 14                        //
#define OLED_SDA_PIN 12                        //
;                                              //
#define LCD_A0_PIN 27                          // a0/dc
#define LCD_RST_PIN 26                         // reset
#define LCD_CS_PIN 32                          // ss
#define LCD_BACKLIGHT_PIN 25                   // led
;                                              //
#define TOUCH_NUM_PADS 4                       //
#define TOUCH_PAD_0_PIN 4                      //
#define TOUCH_PAD_1_PIN 13                     //
#define TOUCH_PAD_2_PIN 2                      //
#define TOUCH_PAD_3_PIN 33                     //
;                                              //
#define SD_CS_PIN 5                            //
;                                              //
;
;
#endif