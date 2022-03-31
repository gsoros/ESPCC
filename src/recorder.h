#ifndef __recorder_h
#define __recorder_h

#include "definitions.h"
#include "atoll_recorder.h"

class Recorder : public Atoll::Recorder {
    bool rec2gpx(const char *in, const char *out);
};

#endif