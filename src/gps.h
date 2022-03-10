#ifndef __gps_h
#define __gps_h

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

#include "task.h"

class GPS : public Task {
   public:
    SoftwareSerial ss;
    TinyGPSPlus gps;

    GPS() {
    }

    void setup() {
        ss.begin(9600, SWSERIAL_8N1, GPIO_NUM_17, GPIO_NUM_16);
    }

    void loop();
};

#endif