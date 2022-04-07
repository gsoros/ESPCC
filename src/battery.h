#ifndef __battery_h
#define __battery_h

#include "definitions.h"
#include "atoll_battery.h"

class Battery : public Atoll::Battery {
   public:
    virtual bool report() override;
};

#endif