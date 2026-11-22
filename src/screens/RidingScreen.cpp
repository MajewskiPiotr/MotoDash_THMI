#include "RidingScreen.h"
#include "Theme.h" // Wspólna paleta kolorów
#include "Config.h"


// Rysuje statyczny układ kafelków (wywoływane raz przy wejściu na Ekran 1)
void RidingScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    tft.fillScreen(BG_COLOR);

    // Nagłówek ekranu w lewym górnym rogu
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    tft.drawString("TRYB JAZDY", 10, 6, 1);

    // --- GÓRNY RZĄD (2 duże kafelki po 148 px szerokości) ---
    // Lewy górny: Temperatura silnika
    tft.fillRoundRect(8, 22, 148, 84, 7, CARD_COLOR);
    tft.drawRoundRect(8, 22, 148, 84, 7, BORDER_COLOR);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.drawString("TEMP. SILNIKA", 16, 30, 1);

    // Prawy górny: Spalanie (chwilowe po lewej, średnie z baku po prawej)
    tft.fillRoundRect(164, 22, 148, 84, 7, CARD_COLOR);
    tft.drawRoundRect(164, 22, 148, 84, 7, BORDER_COLOR);
    tft.drawString("SPALANIE", 172, 30, 1);
    tft.drawString("CHWILOWE", 172, 42, 1);
    tft.drawRightString("SREDNIE", 304, 42, 1);
    tft.drawRightString("L/100km", 304, 88, 1);

    // --- ŚRODKOWY RZĄD ---
    // Lewy dolny (podzielony na 2 mniejsze kafelki po 71 px: Akumulator + Otoczenie)
    tft.fillRoundRect(8, 112, 71, 78, 7, CARD_COLOR);
    tft.drawRoundRect(8, 112, 71, 78, 7, BORDER_COLOR);
    tft.drawCentreString("AKU", 43, 120, 1);

    tft.fillRoundRect(85, 112, 71, 78, 7, CARD_COLOR);
    tft.drawRoundRect(85, 112, 71, 78, 7, BORDER_COLOR);
    tft.drawCentreString("OTOCZ.", 120, 120, 1);

    // Prawy dolny: Paliwo w baku (po lewej) i zasięg (po prawej)
    tft.fillRoundRect(164, 112, 148, 78, 7, CARD_COLOR);
    tft.drawRoundRect(164, 112, 148, 78, 7, BORDER_COLOR);
    tft.drawString("PALIWO / ZASIEG", 172, 120, 1);

    // --- DOLNA BELKA (Y: 196..234) ---
    // Ciśnienie i temperatura opon (TPMS)
    tft.fillRoundRect(8, 196, 304, 38, 7, CARD_COLOR);
    tft.drawRoundRect(8, 196, 304, 38, 7, BORDER_COLOR);
}

// Odświeża dynamiczne wartości liczbowe w pętli głównej
void RidingScreen::update(TFT_eSPI& tft, const MotoState& state) {
    // 1. Temperatura silnika (duża czcionka nr 6 - same wyraźne cyfry + symbol C)
    uint16_t tCol = (state.engineTempC > Config::ENGINE_TEMP_WARN) ? ACCENT_RED : TFT_WHITE;
    tft.setTextColor(tCol, CARD_COLOR);
    tft.setTextPadding(130);
    tft.drawString(String(state.engineTempC) + " C", 16, 56, 4);

    // 2. Spalanie: chwilowe (lewa kolumna) i średnie z baku (prawa kolumna)
    tft.setTextColor(TFT_WHITE, CARD_COLOR);
    tft.setTextPadding(66);
    tft.drawString(String(state.instFuel100, 1), 172, 56, 4);
    tft.setTextColor(ACCENT_GREEN, CARD_COLOR);
    tft.drawRightString(String(state.tankAvgFuel100, 1), 304, 56, 4);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.setTextPadding(60);
    tft.drawString(state.speedKmh > 3 ? "L/100km" : "L/h", 172, 88, 1); // Na postoju chwilowe w L/h
    tft.setTextPadding(0);

    // 3. Akumulator (wyśrodkowany w lewym kafelku)
    tft.setTextColor(ACCENT_GREEN, CARD_COLOR);
    tft.setTextPadding(60);
    tft.drawCentreString(String(state.voltageV, 1) + "V", 43, 148, 2);

    // 4. Temperatura otoczenia (wyśrodkowana, błękitna poniżej 3.0°C)
    uint16_t ambCol = (state.ambientTempC < 3.0f) ? ACCENT_BLUE : TFT_WHITE;
    tft.setTextColor(ambCol, CARD_COLOR);
    tft.setTextPadding(60);
    tft.drawCentreString(String(state.ambientTempC, 1) + "C", 120, 148, 2);

    // 5. Paliwo w baku (lewa kolumna) i szacowany zasięg (prawa kolumna)
    float fuelRatio = (state.tankCapacityL > 0.0f) ? state.fuelInTankL / state.tankCapacityL : 0.0f;
    uint16_t fuelCol = (fuelRatio < 0.10f) ? ACCENT_RED      // Rezerwa krytyczna
                     : (fuelRatio < 0.25f) ? ACCENT_YELLOW   // Niski poziom
                     : TFT_WHITE;
    tft.setTextColor(fuelCol, CARD_COLOR);
    tft.setTextPadding(66);
    tft.drawString(String(state.fuelInTankL, 1), 172, 146, 4);   // Wyśrodkowane w pionie kafelka

    tft.setTextColor(ACCENT_BLUE, CARD_COLOR);
    tft.drawRightString(String(state.rangeKm), 304, 146, 4);
    tft.setTextPadding(0);

    // 6. Dolny pasek TPMS
    // Alarm tylko dla czujników z aktualnym odczytem (brak danych to nie spadek ciśnienia)
    bool alert = (state.frontTpmsOk && state.frontBar < state.minFrontBar) ||
                 (state.rearTpmsOk  && state.rearBar  < state.minRearBar);
    uint16_t bg = alert ? ACCENT_RED : CARD_COLOR;
    uint16_t fg = alert ? TFT_WHITE : ACCENT_GREEN;

    tft.setTextPadding(135);
    tft.setTextColor(fg, bg);
    tft.setTextColor(state.frontTpmsOk ? fg : (alert ? TFT_WHITE : TEXT_MUTED), bg);
    tft.drawString(state.frontTpmsOk ? "F: " + String(state.frontBar, 1) + "b (" + String(state.frontTempC) + "C)"
                                     : String("F: --"), 16, 208, 2);
    tft.setTextColor(state.rearTpmsOk ? fg : (alert ? TFT_WHITE : TEXT_MUTED), bg);
    tft.drawString(state.rearTpmsOk ? "R: " + String(state.rearBar, 1) + "b (" + String(state.rearTempC) + "C)"
                                    : String("R: --"), 170, 208, 2);
}

// Długie przytrzymanie przycisku ACTION na ekranie jazdy resetuje paliwo po tankowaniu
bool RidingScreen::onActionLongPress(MotoState& state) {
    // Szybkie "do pełna" bez pytania o litry - trafia do historii tankowań jako bak bez pomiaru
    if (refuelScreen) refuelScreen->fullWithoutPump(state);
    return true;
}

// Podwójne kliknięcie ACTION na ekranie jazdy otwiera ekran Tankowania (stan baku, do pełna / dolej X litrów)
bool RidingScreen::onActionDoubleClick(MotoState& state) {
    if (!refuelScreen) return false;
    refuelScreen->open(state);
    return true;
}
