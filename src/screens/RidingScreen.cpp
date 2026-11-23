#include "RidingScreen.h"
#include "Theme.h"      // Wspólna paleta kolorów
#include "Icons.h"
#include "Fonts.h"
#include "Config.h"

using SmoothText::LEFT;
using SmoothText::CENTER;
using SmoothText::RIGHT;

// Kolory tego ekranu (styl z ikonami)
static const uint16_t BG   = MODERN_BG;
static const uint16_t CARD = MODERN_CARD;

static void card(TFT_eSPI& tft, int x, int y, int w, int h, uint16_t c = CARD) {
    tft.fillRoundRect(x, y, w, h, CARD_RADIUS, c);
}

// Podpis gładką czcionką (statyczny, rysowany raz)
static void label(TFT_eSPI& tft, const char* txt, int x, int y, int w,
                  SmoothText::Align align = LEFT, uint16_t fg = TEXT_MUTED, uint16_t bg = CARD) {
    SmoothText::draw(tft, FontLabel, txt, x, y, w, 14, align, fg, bg);
}

// Rysuje statyczny układ kart z ikonami (raz przy wejściu na ekran). Bez jednostek - znaczenie dają ikony.
void RidingScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    lastTempBarW = -1; lastFuelSegs = -1; lastFuelCol = 0;
    lastTireState[0] = lastTireState[1] = -1;
    fTemp.reset(); fInst.reset(); fAvg.reset(); fVolt.reset(); fAmb.reset(); fFuel.reset(); fRange.reset();
    for (int i = 0; i < 2; i++) { fTireBar[i].reset(); fTireTemp[i].reset(); }

    tft.fillScreen(BG);
    label(tft, "JAZDA", 10, 3, 60, LEFT, TEXT_MUTED, BG);

    // Górny rząd: silnik | spalanie
    card(tft, 8, 22, 150, 84);
    Icons::thermometer(tft, 16, 30, ACCENT_ORANGE, CARD);
    label(tft, "SILNIK", 40, 33, 70);

    card(tft, 162, 22, 150, 84);
    Icons::droplet(tft, 170, 30, ACCENT_BLUE);
    label(tft, "SPALANIE", 194, 33, 80);
    label(tft, "SREDNIO", 252, 58, 56, RIGHT);

    // Środkowy rząd: akumulator | otoczenie | paliwo + zasięg
    card(tft, 8, 112, 72, 78);
    Icons::battery(tft, 34, 122, ACCENT_GREEN, CARD);
    card(tft, 86, 112, 72, 78);
    Icons::cloud(tft, 112, 122, TFT_WHITE);

    card(tft, 162, 112, 150, 78);
    Icons::fuelPump(tft, 170, 120, ACCENT_YELLOW, CARD);
    label(tft, "PALIWO", 192, 123, 48);
    Icons::road(tft, 242, 120, ACCENT_BLUE, CARD);
    label(tft, "ZASIEG", 264, 123, 44, RIGHT);
}

// Odświeża wartości (co ~150 ms) - każde pole rysuje się tylko po zmianie
void RidingScreen::update(TFT_eSPI& tft, const MotoState& state) {
    // 1. Silnik: duża liczba ze znakiem stopnia + pasek zakresu 40..120 C
    uint16_t tCol = (state.engineTempC > Config::ENGINE_TEMP_WARN) ? ACCENT_RED : TFT_WHITE;
    fTemp.show(tft, FontNumXL, String(state.engineTempC) + "\xC2\xB0", 16, 50, 134, 44, LEFT, tCol, CARD);

    const int barX = 18, barY = 96, barW = 130;
    float r = constrain((state.engineTempC - 40) / 80.0f, 0.0f, 1.0f);
    int w = max(4, (int)(barW * r));
    if (w != lastTempBarW) {
        lastTempBarW = w;
        uint16_t c = state.engineTempC < 95 ? ACCENT_GREEN : (state.engineTempC <= Config::ENGINE_TEMP_WARN ? ACCENT_YELLOW : ACCENT_RED);
        tft.fillRoundRect(barX, barY, barW, 4, 2, TRACK_DIM);
        tft.fillRoundRect(barX, barY, w, 4, 2, c);
    }

    // 2. Spalanie: chwilowe duże (od 10 bez miejsca po przecinku), średnie z baku mniejsze z boku
    float inst = state.instFuel100;
    fInst.show(tft, FontNumXL, inst >= 10.0f ? String((int)roundf(inst)) : String(inst, 1),
               168, 50, 84, 44, LEFT, TFT_WHITE, CARD);
    fAvg.show(tft, FontNumM, String(state.tankAvgFuel100, 1), 252, 72, 56, 18, RIGHT, ACCENT_GREEN, CARD);

    // 3. Akumulator i temperatura otoczenia (niebieska poniżej 3 C - ryzyko gołoledzi)
    fVolt.show(tft, FontNumL, String(state.voltageV, 1), 8, 150, 72, 26, CENTER, TFT_WHITE, CARD);
    fAmb.show(tft, FontNumL, String((int)roundf(state.ambientTempC)) + "\xC2\xB0", 86, 150, 72, 26, CENTER,
              state.ambientTempC < 3.0f ? ACCENT_BLUE : TFT_WHITE, CARD);

    // 4. Paliwo i zasięg + pasek 10 segmentów
    float ratio = state.tankCapacityL > 0 ? state.fuelInTankL / state.tankCapacityL : 0;
    uint16_t fCol = ratio < 0.10f ? ACCENT_RED : (ratio < 0.25f ? ACCENT_YELLOW : TFT_WHITE);
    fFuel.show(tft, FontNumL, String(state.fuelInTankL, 1), 170, 144, 70, 26, LEFT, fCol, CARD);
    fRange.show(tft, FontNumL, String(state.rangeKm), 240, 144, 64, 26, RIGHT, ACCENT_BLUE, CARD);

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
        fTireBar[idx].reset(); fTireTemp[idx].reset();
        card(tft, x, 196, 150, 40, bg);
        Icons::tire(tft, x + 8, 206, alert ? TFT_WHITE : (ok ? ACCENT_GREEN : TEXT_MUTED), bg);
        label(tft, idx == 0 ? "PRZOD" : "TYL", x + 36, 199, 60, LEFT, txtC, bg);
        if (!ok) SmoothText::draw(tft, FontNumM, "-", x + 36, 213, 50, 18, LEFT, TEXT_MUTED, bg);
    }
    if (ok) {
        fTireBar[idx].show(tft, FontNumM, String(bar, 1), x + 36, 213, 50, 18, LEFT, TFT_WHITE, bg);
        fTireTemp[idx].show(tft, FontNumM, String(tempC) + "\xC2\xB0", x + 92, 213, 50, 18, RIGHT, txtC, bg);
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
