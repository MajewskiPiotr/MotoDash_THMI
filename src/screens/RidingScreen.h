#pragma once
#include "Screen.h"
#include "FuelCalculator.h"
#include "RefuelScreen.h"
#include "SmoothText.h"

// Ekran 1: Główny tryb jazdy w stylu z ikonami (temperatura, spalanie, akumulator, otoczenie, paliwo, opony).
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

    // Zapamiętany stan elementów graficznych - rysujemy je ponownie tylko po zmianie (bez migania)
    int  lastTempBarW  = -1;
    int  lastFuelSegs  = -1;
    uint16_t lastFuelCol = 0;
    int8_t lastTireState[2] = {-1, -1}; // 0 = brak odczytu, 1 = OK, 2 = alarm

    // Pola z wartościami (gładka czcionka, rysowane tylko po zmianie)
    SmoothText::Field fTemp, fInst, fAvg, fVolt, fAmb, fFuel, fRange, fTireBar[2], fTireTemp[2];

    void drawTire(TFT_eSPI& tft, int idx, bool ok, float bar, int tempC, float minBar);
};
