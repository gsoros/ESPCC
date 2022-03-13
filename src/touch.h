#ifndef __touch_h
#define __touch_h

#include "definitions.h"

#define ATOLL_TOUCH_NUM_PADS TOUCH_NUM_PADS
#include "atoll_touch.h"

typedef Atoll::TouchPad TouchPad;
typedef Atoll::TouchEvent TouchEvent;

class Touch : public Atoll::Touch {
   public:
    Touch(int pin0 = -1,
          int pin1 = -1,
          int pin2 = -1,
          int pin3 = -1) : Atoll::Touch(pin0, pin1, pin2, pin3){};

   protected:
    virtual void onEvent(uint8_t index, TouchEvent event);
};

#endif