#pragma once
#include "Screen.h"
#include "CanReader.h"

// Ekran 2: Statystyki motocykla (Max RPM, Max Temp, Min napięcie startowe, błędy ECU).
class MotoStatsScreen : public Screen {
public:
    explicit MotoStatsScreen(CanReader* can) : canReader(can) {}

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;
    bool onModeLongPress(MotoState& state) override;   // Reset wartości Max/Min
    bool onActionLongPress(MotoState& state) override; // Kasowanie błędów ECU

private:
    CanReader* canReader;
};
