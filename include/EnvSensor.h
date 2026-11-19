#pragma once
#include "MotoState.h"

class EnvSensor {
public:
    void init();
    void update(MotoState& state);

private:
    unsigned long lastReadMs = 0;
};
