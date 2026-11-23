#include "RidingScreen.h"
#include "Theme.h" // Wspólna paleta kolorów
#include "Icons.h"
#include "Config.h"

// Kolory tego ekranu (styl z ikonami)
static const uint16_t BG   = MODERN_BG;
static const uint16_t CARD = MODERN_CARD;

static void card(TFT_eSPI& tft, int x, int y, int w, int h, uint16_t c = CARD) {
    tft.fillRoundRect(x, y, w, h, 10, c);
}

static void label(TFT_eSPI& tft, const char* txt, int x, int y, uint16_t bg = CARD) {
    tft.setTextColor(TEXT_MUTED, bg);
    tft.drawString(txt, x, y, 1);
}

// Rysuje statyczny układ kart z ikonami (raz przy wejściu na ekran)
void RidingScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    lastTempBarW = -1; lastFuelSegs = -1; lastFuelCol = 0;
    lastTireState[0] = lastTireState[1] = -1;

    tft.fillScreen(BG);
    tft.setTextColor(TEXT_MUTED, BG);
    tft.drawString("JAZDA", 10, 6, 1);

    // Górny rząd: silnik | spalanie
    card(tft, 8, 22, 150, 84);
    Icons::thermometer(tft, 16, 30, ACCENT_ORANGE, CARD);
    label(tft, "SILNIK", 40, 36);
    tft.setTextColor(TEXT_MUTED, CARD);
    tft.drawString("C", 112, 54, 2);

    card(tft, 162, 22, 150, 84);
    Icons::droplet(tft, 170, 30, ACCENT_BLUE);
    label(tft, "SPALANIE", 194, 36);
    label(tft, "sr.", 272, 72);

    // Środkowy rząd: akumulator | otoczenie | paliwo + zasięg
    card(tft, 8, 112, 72, 78);
    Icons::battery(tft, 34, 120, ACCENT_GREEN, CARD);
    tft.setTextColor(TEXT_MUTED, CARD);
    tft.drawCentreString("V", 44, 172, 2);

    card(tft, 86, 112, 72, 78);
    Icons::cloud(tft, 112, 120, TFT_WHITE);
    tft.setTextColor(TEXT_MUTED, CARD);
    tft.drawString("C", 123, 172, 2);
    Icons::degree(tft, 118, 175, TEXT_MUTED);

    card(tft, 162, 112, 150, 78);
    Icons::fuelPump(tft, 170, 120, ACCENT_YELLOW, CARD);
    label(tft, "PALIWO", 194, 126);
    Icons::road(tft, 246, 120, ACCENT_BLUE, CARD);
    label(tft, "ZASIEG", 270, 126);
    tft.setTextColor(TEXT_MUTED, CARD);
    tft.drawString("L", 229, 150, 2);
    tft.drawString("km", 290, 150, 1);
}

// Odświeża wartości (co ~150 ms); grafiki tylko po zmianie
void RidingScreen::update(TFT_eSPI& tft, const MotoState& state) {
    // 1. Silnik: liczba w stałym polu (wyrównana do prawej), znak stopnia, pasek zakresu 40..120 C
    uint16_t tCol = (state.engineTempC > Config::ENGINE_TEMP_WARN) ? ACCENT_RED : TFT_WHITE;
    tft.setTextColor(tCol, CARD);
    tft.setTextPadding(84);
    tft.drawRightString(String(state.engineTempC), 102, 50, 6);
    tft.setTextPadding(0);
    Icons::degree(tft, 108, 56, tCol);

    const int barX = 18, barY = 96, barW = 130;
    float r = constrain((state.engineTempC - 40) / 80.0f, 0.0f, 1.0f);
    int w = max(4, (int)(barW * r));
    if (w != lastTempBarW) {
        lastTempBarW = w;
        uint16_t c = state.engineTempC < 95 ? ACCENT_GREEN : (state.engineTempC <= Config::ENGINE_TEMP_WARN ? ACCENT_YELLOW : ACCENT_RED);
        tft.fillRoundRect(barX, barY, barW, 4, 2, TRACK_DIM);
        tft.fillRoundRect(barX, barY, w, 4, 2, c);
    }

    // 2. Spalanie: chwilowe duże (od 10 bez miejsca po przecinku), jednostka i średnia z baku obok
    float inst = state.instFuel100;
    tft.setTextColor(TFT_WHITE, CARD);
    tft.setTextPadding(96);
    tft.drawRightString(inst >= 10.0f ? String((int)roundf(inst)) : String(inst, 1), 266, 50, 6);
    tft.setTextColor(TEXT_MUTED, CARD);
    tft.setTextPadding(34);
    tft.drawString(state.speedKmh > 3 ? "L/100" : "L/h", 272, 56, 1);
    tft.setTextColor(ACCENT_GREEN, CARD);
    tft.setTextPadding(36);
    tft.drawString(String(state.tankAvgFuel100, 1), 272, 82, 2);

    // 3. Akumulator i temperatura otoczenia (niebieska poniżej 3 C - ryzyko gołoledzi)
    tft.setTextPadding(64);
    tft.setTextColor(TFT_WHITE, CARD);
    tft.drawCentreString(String(state.voltageV, 1), 44, 146, 4);
    tft.setTextColor(state.ambientTempC < 3.0f ? ACCENT_BLUE : TFT_WHITE, CARD);
    tft.drawCentreString(String((int)roundf(state.ambientTempC)), 122, 146, 4);

    // 4. Paliwo i zasięg + pasek 10 segmentów
    float ratio = state.tankCapacityL > 0 ? state.fuelInTankL / state.tankCapacityL : 0;
    uint16_t fCol = ratio < 0.10f ? ACCENT_RED : (ratio < 0.25f ? ACCENT_YELLOW : TFT_WHITE);
    tft.setTextColor(fCol, CARD);
    tft.setTextPadding(58);
    tft.drawRightString(String(state.fuelInTankL, 1), 226, 142, 4);
    tft.setTextColor(ACCENT_BLUE, CARD);
    tft.setTextPadding(48);
    tft.drawRightString(String(state.rangeKm), 288, 142, 4);
    tft.setTextPadding(0);

    const int segs = 10, sx = 170, sy = 176, sw = 134, gap = 2;
    const int segW = (sw - (segs - 1) * gap) / segs;
    int n = (int)roundf(constrain(ratio, 0.0f, 1.0f) * segs);
    uint16_t sCol = ratio < 0.10f ? ACCENT_RED : (ratio < 0.25f ? ACCENT_YELLOW : ACCENT_GREEN);
    if (n != lastFuelSegs || sCol != lastFuelCol) {
        lastFuelSegs = n; lastFuelCol = sCol;
        for (int i = 0; i < segs; i++)
            tft.fillRoundRect(sx + i * (segW + gap), sy, segW, 6, 2, i < n ? sCol : TRACK_DIM);
    }

    // 5. Opony (alarm tylko dla czujników z aktualnym odczytem)
    drawTire(tft, 0, state.frontTpmsOk, state.frontBar, state.frontTempC, state.minFrontBar);
    drawTire(tft, 1, state.rearTpmsOk,  state.rearBar,  state.rearTempC,  state.minRearBar);
}

void RidingScreen::drawTire(TFT_eSPI& tft, int idx, bool ok, float bar, int tempC, float minBar) {
    const int x = 8 + idx * 154;
    bool alert = ok && bar < minBar;
    int8_t st = !ok ? 0 : (alert ? 2 : 1);
    uint16_t bg   = alert ? ACCENT_RED : CARD;
    uint16_t txtC = alert ? TFT_WHITE : TEXT_MUTED;

    // Tło karty, ikona i podpis - tylko gdy zmienił się stan (brak / OK / alarm)
    if (st != lastTireState[idx]) {
        lastTireState[idx] = st;
        card(tft, x, 196, 150, 40, bg);
        Icons::tire(tft, x + 8, 206, alert ? TFT_WHITE : (ok ? ACCENT_GREEN : TEXT_MUTED), bg);
        tft.setTextColor(txtC, bg);
        tft.drawString(idx == 0 ? "PRZOD" : "TYL", x + 36, 202, 1);
        if (ok) {
            tft.drawString("bar", x + 64, 217, 1);
            tft.drawString("C", x + 138, 213, 2);
            Icons::degree(tft, x + 134, 216, txtC);
        } else {
            tft.setTextColor(TEXT_MUTED, bg);
            tft.drawString("--", x + 36, 213, 2);
            tft.drawString("brak odczytu", x + 60, 217, 1);
        }
    }
    if (ok) {
        tft.setTextColor(TFT_WHITE, bg);
        tft.setTextPadding(26);
        tft.drawString(String(bar, 1), x + 36, 213, 2);
        tft.setTextColor(txtC, bg);
        tft.setTextPadding(36);
        tft.drawRightString(String(tempC), x + 132, 213, 2);
        tft.setTextPadding(0);
    }
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
