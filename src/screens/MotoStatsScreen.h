#pragma once
#include "Screen.h"
#include "CanReader.h"
#include "SmoothText.h"

// Ekran 2: Statystyki motocykla - rekordy od zerowania (prędkość, obroty, temperatura, napięcie przy rozruchu) i błędy ECU.
class MotoStatsScreen : public Screen {
public:
    explicit MotoStatsScreen(CanReader* can) : canReader(can) {}

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;
    bool onLongTouch(MotoState& state, int x, int y) override; // Przytrzymanie karty: zeruj rekordy / kasuj błędy

private:
    CanReader* canReader;
    // Wartości w gładkiej czcionce - rysowane tylko po zmianie
    SmoothText::Field fSpeed, fRpm, fTemp, fVolt, fDtc, fDtcNote;
};
