#ifndef __recorder_h
#define __recorder_h

#include "definitions.h"
#include "atoll_recorder.h"

class Recorder : public Atoll::Recorder {
    void onDistanceChanged(double value);
    void onAltGainChanged(uint16_t value);
    bool rec2gpx(const char *in, const char *out);
};

#endif