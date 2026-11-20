#include "MotoStatsScreen.h"

#define BG_COLOR     0x0841 // Ciemny grafit (tło główne)
#define CARD_COLOR   0x10A3 // Ciemnoszary (wnętrze kafelków)
#define BORDER_COLOR 0x2965 // Szary (ramki kafelków)
#define TEXT_MUTED   0x8C71 // Jasnoszary (opisy i legenda)
#define ACCENT_GREEN 0x07E0 // Zielony (prawidłowe wartości)
#define ACCENT_BLUE  0x05FF // Błękitny (nagłówek ekranu i prędkość max)
#define ACCENT_RED   0xF800 // Czerwony (alarm słabej baterii lub błędów ECU)

// Rysuje statyczny układ 4 kafelków ze statystykami motocykla
void MotoStatsScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    tft.fillScreen(BG_COLOR);

    tft.setTextColor(ACCENT_BLUE, BG_COLOR);
    tft.drawString("STATYSTYKI MOTOCYKLA", 10, 6, 1);

    // Kafelek 1 (Lewy górny): Prędkość Max oraz Obroty Max
    tft.fillRoundRect(8, 22, 148, 88, 7, CARD_COLOR);
    tft.drawRoundRect(8, 22, 148, 88, 7, BORDER_COLOR);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.drawString("MAX V / MAX RPM", 16, 30, 1);

    // Kafelek 2 (Prawy górny): Najwyższa temperatura silnika
    tft.fillRoundRect(164, 22, 148, 88, 7, CARD_COLOR);
    tft.drawRoundRect(164, 22, 148, 88, 7, BORDER_COLOR);
    tft.drawString("MAX TEMP. SILNIKA", 172, 30, 1);

    // Kafelek 3 (Lewy dolny): Najniższe napięcie przy rozruchu
    tft.fillRoundRect(8, 118, 148, 88, 7, CARD_COLOR);
    tft.drawRoundRect(8, 118, 148, 88, 7, BORDER_COLOR);
    tft.drawString("MIN NAPIECIE (START)", 16, 126, 1);

    // Kafelek 4 (Prawy dolny): Błędy ECU (DTC)
    tft.fillRoundRect(164, 118, 148, 88, 7, CARD_COLOR);
    tft.drawRoundRect(164, 118, 148, 88, 7, BORDER_COLOR);
    tft.drawString("BLEDY ECU (DTC)", 172, 126, 1);

    // Dolna legenda przycisków
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    tft.drawString("[MODE 2s]: Reset Max/Min", 10, 218, 1);
    tft.drawString("[ACT 2s]: Kasuj bledy", 185, 218, 1);
}

// Aktualizuje liczby w 4 kafelkach statystyk
void MotoStatsScreen::update(TFT_eSPI& tft, const MotoState& state) {
    tft.setTextPadding(132); // Zapobiega nakładaniu się cyfr

    // 1. Prędkość maksymalna (duża) oraz Maksymalne obroty (poniżej)
    tft.setTextColor(ACCENT_BLUE, CARD_COLOR);
    tft.drawString(String(state.maxSpeedKmh) + " km/h", 16, 48, 4);

    tft.setTextColor(TFT_WHITE, CARD_COLOR);
    tft.drawString(String(state.maxRpm) + " rpm", 16, 82, 2);

    // 2. Maksymalna temperatura silnika
    tft.drawString(String(state.maxEngineTempC) + " C", 172, 62, 4);

    // 3. Minimalne napięcie akumulatora przy rozruchu
    uint16_t battCol = (state.minBootVoltageV < 10.5f) ? ACCENT_RED : ACCENT_GREEN;
    tft.setTextColor(battCol, CARD_COLOR);
    tft.drawString(String(state.minBootVoltageV, 1) + " V", 16, 158, 4);

    // 4. Liczba błędów sterownika ECU
    uint16_t dtcCol = (state.dtcCount > 0) ? ACCENT_RED : ACCENT_GREEN;
    tft.setTextColor(dtcCol, CARD_COLOR);
    String dtcText = (state.dtcCount == 0) ? "0 (OK)" : String(state.dtcCount) + " BLEDY";
    tft.drawString(dtcText, 172, 158, 4);
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
