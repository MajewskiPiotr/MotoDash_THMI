#pragma once
#include "MotoState.h"

class TpmsScanner {
public:
    void init();
    void update(MotoState& state);

private:
    unsigned long lastScanMs = 0;
};
