#pragma once
#include "Screen.h"
#include "FuelCalculator.h"
#include "RefuelScreen.h"

// Ekran 1: Główny tryb jazdy (bieg, temperatura, napięcie, spalanie, zasięg, TPMS).
class RidingScreen : public Screen {
public:
    // Konstruktor przyjmujący wskaźnik na kalkulator paliwa (Dependency Injection)
    RidingScreen(FuelCalculator* fuelCalc, RefuelScreen* refuel)
        : fuelCalculator(fuelCalc), refuelScreen(refuel) {}

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;
    bool onActionLongPress(MotoState& state) override; // Obsługa resetu po tankowaniu
    bool onActionDoubleClick(MotoState& state) override; // Podwójne kliknięcie ACTION otwiera ekran Tankowania

private:
    FuelCalculator* fuelCalculator;
    RefuelScreen*   refuelScreen;
};
