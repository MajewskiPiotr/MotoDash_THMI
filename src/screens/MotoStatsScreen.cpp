#include "MotoStatsScreen.h"
#include "Theme.h"
#include "SmoothText.h"
#include "Icons.h"
#include "Config.h" // Wspólna paleta kolorów


// Rysuje statyczny układ 4 kafelków ze statystykami motocykla
void MotoStatsScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    tft.fillScreen(BG_COLOR);
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    SmoothText::str(tft, "STATYSTYKI", 10, 6, 1);

    // 4 karty z ikonami: prędkość/obroty | temperatura | napięcie przy rozruchu | błędy ECU
    tft.fillRoundRect(8, 22, 148, 88, CARD_RADIUS, CARD_COLOR);
    Icons::speedometer(tft, 16, 30, ACCENT_BLUE, CARD_COLOR);
    tft.fillRoundRect(164, 22, 148, 88, CARD_RADIUS, CARD_COLOR);
    Icons::thermometer(tft, 172, 30, ACCENT_ORANGE, CARD_COLOR);
    tft.fillRoundRect(8, 118, 148, 88, CARD_RADIUS, CARD_COLOR);
    Icons::battery(tft, 16, 126, ACCENT_GREEN, CARD_COLOR);
    tft.fillRoundRect(164, 118, 148, 88, CARD_RADIUS, CARD_COLOR);
    Icons::warning(tft, 172, 126, ACCENT_YELLOW, CARD_COLOR);

    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    SmoothText::str(tft, "MAX V / RPM", 40, 36, 1);
    SmoothText::str(tft, "MAX TEMP.", 196, 36, 1);
    SmoothText::str(tft, "MIN NAPIĘCIE", 40, 132, 1);
    SmoothText::str(tft, "BŁĘDY ECU", 196, 132, 1);

    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    SmoothText::str(tft, "[MODE 2s]: Reset Max/Min", 10, 218, 1);
    SmoothText::str(tft, "[ACT 2s]: Kasuj błędy", 185, 218, 1);
}

// Aktualizuje liczby w 4 kartach statystyk
void MotoStatsScreen::update(TFT_eSPI& tft, const MotoState& state) {
    tft.setTextPadding(132); // Zapobiega nakładaniu się cyfr

    // 1. Prędkość maksymalna (duża) oraz maksymalne obroty (poniżej)
    tft.setTextColor(ACCENT_BLUE, CARD_COLOR);
    SmoothText::str(tft, String(state.maxSpeedKmh) + " km/h", 16, 56, 4);
    tft.setTextColor(TFT_WHITE, CARD_COLOR);
    SmoothText::str(tft, String(state.maxRpm) + " rpm", 16, 88, 2);

    // 2. Maksymalna temperatura silnika
    tft.setTextColor(state.maxEngineTempC > Config::ENGINE_TEMP_WARN ? ACCENT_RED : TFT_WHITE, CARD_COLOR);
    SmoothText::str(tft, String(state.maxEngineTempC) + " C", 172, 62, 4);

    // 3. Minimalne napięcie akumulatora przy rozruchu
    uint16_t battCol = (state.minBootVoltageV < 10.5f) ? ACCENT_RED : ACCENT_GREEN;
    tft.setTextColor(battCol, CARD_COLOR);
    SmoothText::str(tft, String(state.minBootVoltageV, 1) + " V", 16, 158, 4);

    // 4. Liczba błędów sterownika ECU
    uint16_t dtcCol = (state.dtcCount > 0) ? ACCENT_RED : ACCENT_GREEN;
    tft.setTextColor(dtcCol, CARD_COLOR);
    String dtcText = (state.dtcCount == 0) ? "0 (OK)" : String(state.dtcCount) + " BŁĘDY";
    SmoothText::str(tft, dtcText, 172, 158, 4);
    tft.setTextPadding(0);
}

bool MotoStatsScreen::onModeLongPress(MotoState& state) {
    if (canReader) {
        canReader->resetStats(state);
        state.forceRedraw = true;
    }
    return true;
}

bool MotoStatsScreen::onActionLongPress(MotoState& state) {
    if (canReader) {
        canReader->clearDtcCodes(state);
        state.forceRedraw = true;
    }
    return true;
}
