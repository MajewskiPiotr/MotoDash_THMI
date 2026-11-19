#pragma once
#include "Screen.h"
#include "FuelCalculator.h"

// Ekran 1: GĹ‚Ăłwny tryb jazdy (bieg, temperatura, napiÄ™cie, spalanie, zasiÄ™g, TPMS).
class RidingScreen : public Screen {
public:
    // Konstruktor przyjmujÄ…cy wskaĹşnik na kalkulator paliwa (Dependency Injection)
    explicit RidingScreen(FuelCalculator* fuelCalc) : fuelCalculator(fuelCalc) {}

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;
    bool onActionLongPress(MotoState& state) override; // ObsĹ‚uga resetu po tankowaniu

private:
    FuelCalculator* fuelCalculator;
};
