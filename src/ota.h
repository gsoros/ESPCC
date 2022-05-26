#ifndef __ota_h
#define __ota_h

#include "atoll_ota.h"

class Ota : public Atoll::Ota {
   public:
    virtual void onStart() override;
    virtual void onEnd() override;
    virtual void onProgress(uint progress, uint total) override;
    virtual void onError(ota_error_t error) override;
};

#endif