#ifndef __gps_h
#define __gps_h

#include <SoftwareSerial.h>
#include <TinyGPS++.h>

#include "definitions.h"
#include "atoll_task.h"

class GPS : public Atoll::Task {
   public:
    SoftwareSerial ss;
    TinyGPSPlus gps;

    GPS() {
    }

    void setup() {
        ss.begin(9600, SWSERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    }

    void loop();
};

#endif