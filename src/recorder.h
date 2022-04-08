#ifndef __recorder_h
#define __recorder_h

#include "definitions.h"
#include "atoll_recorder.h"

class Recorder : public Atoll::Recorder {
   public:
    virtual void onDistanceChanged(double value) override;
    virtual void onAltGainChanged(uint16_t value) override;
    virtual bool start() override;
    virtual bool stop(bool forgetLast = false) override;
    virtual bool rec2gpx(const char *in, const char *out) override;
};

#endif