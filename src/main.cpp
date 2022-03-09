#include <Arduino.h>

#include <SoftwareSerial.h>
#include <TinyGPS++.h>

SoftwareSerial ss;
TinyGPSPlus gps;

void setup() {
    Serial.begin(115200);
    Serial.println("--------");
    ss.begin(9600, SWSERIAL_8N1, GPIO_NUM_17, GPIO_NUM_16);
}

void loop() {
    while (ss.available() > 0) {
        char c = ss.read();
        gps.encode(c);
        // Serial.print(c);
    }
    if (gps.location.isUpdated()) {
        Serial.printf("%s Lat=%f Lon=%f Alt=%f HDOP=%d Speed=%f, Sat=%d%s SeFi=%d T=%d:%d:%d%s\n",
                      gps.location.isValid() ? "fix" : "___",
                      gps.location.lat(),  //
                      gps.location.lng(),
                      gps.altitude.meters(),
                      gps.hdop.value(),
                      gps.speed.kmph(),
                      gps.satellites.value(),
                      gps.satellites.isValid() ? "" : "X",
                      gps.sentencesWithFix(),
                      gps.time.hour(),
                      gps.time.minute(),
                      gps.time.second(),
                      gps.time.isValid() ? "" : "X");
    }
}