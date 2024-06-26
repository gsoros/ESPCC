#ifndef __touch_h
#define __touch_h

#include "definitions.h"

#undef ATOLL_TOUCH_NUM_PADS
#define ATOLL_TOUCH_NUM_PADS TOUCH_NUM_PADS
#include "atoll_touch.h"

#define TOUCH_PAD_TOPLEFT 0
#define TOUCH_PAD_TOPRIGHT 1
#define TOUCH_PAD_BOTTOMLEFT 2
#define TOUCH_PAD_BOTTOMRIGHT 3

class Touch : public Atoll::Touch {
   public:
    Touch(
        int pin0 = -1,
        int pin1 = -1,
        int pin2 = -1,
        int pin3 = -1)
        : Atoll::Touch(pin0, pin1, pin2, pin3){};

    virtual void onEnabledChanged() override;

    bool locked = false;

   protected:
    virtual void fireEvent(uint8_t index, Event event) override;
};

#endif