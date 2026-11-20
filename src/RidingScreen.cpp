#include "RidingScreen.h"
#include "Config.h"

// Paleta kolorĂłw w formacie RGB565
#define BG_COLOR     0x0841 // Ciemny grafit (tĹ‚o gĹ‚Ăłwne)
#define CARD_COLOR   0x10A3 // Ciemnoszary (wnÄ™trze kafelkĂłw)
#define BORDER_COLOR 0x2965 // Szary (obramowania kafelkĂłw)
#define TEXT_MUTED   0x8C71 // Przygaszony jasnoszary (etykiety)
#define ACCENT_GREEN 0x07E0 // Zielony (napiÄ™cie akumulatora, poprawne ciĹ›nienie)
#define ACCENT_BLUE  0x05FF // BĹ‚Ä™kitny (zasiÄ™g oraz ostrzeĹĽenie o goĹ‚oledzi)
#define ACCENT_RED   0xF800 // Czerwony (alarm przegrzania silnika lub spadku ciĹ›nienia opon)

// Rysuje statyczny ukĹ‚ad kafelkĂłw (wywoĹ‚ywane raz przy wejĹ›ciu na Ekran 1)
void RidingScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    tft.fillScreen(BG_COLOR);

    // NagĹ‚Ăłwek ekranu w lewym gĂłrnym rogu
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    tft.drawString("TRYB JAZDY", 10, 6, 1);

    // --- GĂ“RNY RZÄ„D (2 duĹĽe kafelki po 148 px szerokoĹ›ci) ---
    // Lewy gĂłrny: Temperatura silnika
    tft.fillRoundRect(8, 22, 148, 84, 7, CARD_COLOR);
    tft.drawRoundRect(8, 22, 148, 84, 7, BORDER_COLOR);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.drawString("TEMP. SILNIKA", 16, 30, 1);

    // Prawy gĂłrny: Spalanie chwilowe
    tft.fillRoundRect(164, 22, 148, 84, 7, CARD_COLOR);
    tft.drawRoundRect(164, 22, 148, 84, 7, BORDER_COLOR);
    tft.drawString("SPALANIE CHWILOWE", 172, 30, 1);

    // --- ĹšRODKOWY RZÄ„D ---
    // Lewy dolny (podzielony na 2 mniejsze kafelki po 71 px: Akumulator + Otoczenie)
    tft.fillRoundRect(8, 112, 71, 78, 7, CARD_COLOR);
    tft.drawRoundRect(8, 112, 71, 78, 7, BORDER_COLOR);
    tft.drawCentreString("AKU", 43, 120, 1);

    tft.fillRoundRect(85, 112, 71, 78, 7, CARD_COLOR);
    tft.drawRoundRect(85, 112, 71, 78, 7, BORDER_COLOR);
    tft.drawCentreString("OTOCZ.", 120, 120, 1);

    // Prawy dolny: Spalanie Ĺ›rednie i pozostaĹ‚y zasiÄ™g
    tft.fillRoundRect(164, 112, 148, 78, 7, CARD_COLOR);
    tft.drawRoundRect(164, 112, 148, 78, 7, BORDER_COLOR);
    tft.drawString("SREDNIE / ZASIEG", 172, 120, 1);
    tft.drawString("2x ACTION: tankowanie", 172, 176, 1); // Podpowiedź otwarcia ekranu Tankowania

    // --- DOLNA BELKA (Y: 196..234) ---
    // CiĹ›nienie i temperatura opon (TPMS)
    tft.fillRoundRect(8, 196, 304, 38, 7, CARD_COLOR);
    tft.drawRoundRect(8, 196, 304, 38, 7, BORDER_COLOR);
}

// OdĹ›wieĹĽa dynamiczne wartoĹ›ci liczbowe w pÄ™tli gĹ‚Ăłwnej
void RidingScreen::update(TFT_eSPI& tft, const MotoState& state) {
    // 1. Temperatura silnika (duĹĽa czcionka nr 6 - same wyraĹşne cyfry + symbol C)
    uint16_t tCol = (state.engineTempC > Config::ENGINE_TEMP_WARN) ? ACCENT_RED : TFT_WHITE;
    tft.setTextColor(tCol, CARD_COLOR);
    tft.setTextPadding(130);
    tft.drawString(String(state.engineTempC) + " C", 16, 56, 4);

    // 2. Spalanie chwilowe (duĹĽa liczba + jednostka obok)
    tft.setTextColor(TFT_WHITE, CARD_COLOR);
    tft.setTextPadding(130);
    tft.drawString(String(state.instFuel100, 1), 172, 52, 4);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.drawString(state.speedKmh > 3 ? "L/100km" : "L/h", 172, 84, 1);

    // 3. Akumulator (wyĹ›rodkowany w lewym kafelku)
    tft.setTextColor(ACCENT_GREEN, CARD_COLOR);
    tft.setTextPadding(60);
    tft.drawCentreString(String(state.voltageV, 1) + "V", 43, 148, 2);

    // 4. Temperatura otoczenia (wyĹ›rodkowana, bĹ‚Ä™kitna poniĹĽej 3.0Â°C)
    uint16_t ambCol = (state.ambientTempC < 3.0f) ? ACCENT_BLUE : TFT_WHITE;
    tft.setTextColor(ambCol, CARD_COLOR);
    tft.setTextPadding(60);
    tft.drawCentreString(String(state.ambientTempC, 1) + "C", 120, 148, 2);

    // 5. Spalanie Ĺ›rednie i pozostaĹ‚y zasiÄ™g obok siebie w prawym dolnym kafelku
    tft.setTextColor(TFT_WHITE, CARD_COLOR);
    tft.setTextPadding(65);
    tft.drawString(String(state.overallAvgFuel100 , 1) + " L", 172, 148, 2);

    tft.setTextColor(ACCENT_BLUE, CARD_COLOR);
    tft.setTextPadding(70);
    tft.drawRightString("~" + String(state.rangeKm) + " km", 304, 148, 2);

    // 6. Dolny pasek TPMS
    bool alert = (state.frontBar < state.minFrontBar || state.rearBar < state.minRearBar);
    uint16_t bg = alert ? ACCENT_RED : CARD_COLOR;
    uint16_t fg = alert ? TFT_WHITE : ACCENT_GREEN;

    tft.setTextPadding(135);
    tft.setTextColor(fg, bg);
    tft.drawString("F: " + String(state.frontBar, 1) + "b (" + String(state.frontTempC) + "C)", 16, 208, 2);
    tft.drawString("R: " + String(state.rearBar, 1) + "b (" + String(state.rearTempC) + "C)", 170, 208, 2);
}

// DĹ‚ugie przytrzymanie przycisku ACTION na ekranie jazdy resetuje paliwo po tankowaniu
bool RidingScreen::onActionLongPress(MotoState& state) {
    if (fuelCalculator) {
        fuelCalculator->resetRefuel(state);
        state.forceRedraw = true;
    }
    return true;
}

// Podwójne kliknięcie ACTION na ekranie jazdy otwiera ekran Tankowania (stan baku, do pełna / dolej X litrów)
bool RidingScreen::onActionDoubleClick(MotoState& state) {
    if (!refuelScreen) return false;
    refuelScreen->open(state);
    return true;
}
