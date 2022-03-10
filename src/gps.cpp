#include "gps.h"
#include "board.h"

void GPS::loop() {
    // Serial.printf("[GPS] loop\n");
    while (ss.available() > 0) {
        char c = ss.read();
        gps.encode(c);
        // Serial.print(c);
    }
    static unsigned long lastPrint = millis();
    if (gps.location.isValid()) {
        board.oled.displayGps(&gps);
        if (millis() < lastPrint + 3000) return;
        lastPrint = millis();
        Serial.printf(
            "%f %f %.1fm %.2fkm/h %d %d%ssat %02d:%02d:%02d%s(%04d) F%d P%d S%d\n",  //
            gps.location.lat(),
            gps.location.lng(),
            gps.altitude.meters(),
            gps.speed.kmph(),
            gps.hdop.value(),
            gps.satellites.value(),
            gps.satellites.isValid() ? "" : "X",
            gps.time.hour(),
            gps.time.minute(),
            gps.time.second(),
            gps.time.isValid() ? "" : "X",
            gps.time.age(),
            gps.failedChecksum(),
            gps.passedChecksum(),
            gps.sentencesWithFix());
        return;
    }
    static uint32_t satellites = UINT32_MAX;
    if (satellites != gps.satellites.value()) {
        satellites = gps.satellites.value();
        board.oled.displaySatellites(satellites);
    }
}