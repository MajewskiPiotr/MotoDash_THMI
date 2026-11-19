#pragma once
#include "Screen.h"
#include "CanReader.h"

// Ekran 2: Statystyki motocykla (Max RPM, Max Temp, Min napiÄ™cie startowe, bĹ‚Ä™dy ECU).
class MotoStatsScreen : public Screen {
public:
    explicit MotoStatsScreen(CanReader* can) : canReader(can) {}

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;
    bool onModeLongPress(MotoState& state) override;   // Reset wartoĹ›ci Max/Min
    bool onActionLongPress(MotoState& state) override; // Kasowanie bĹ‚Ä™dĂłw ECU

private:
    CanReader* canReader;
};
