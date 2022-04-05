#ifndef __gps_h
#define __gps_h

#include "atoll_gps.h"

class GPS : public Atoll::GPS {
   public:
    void loop();
};

#endif