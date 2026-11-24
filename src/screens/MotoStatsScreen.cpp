#include "MotoStatsScreen.h"
#include "Theme.h"
#include "SmoothText.h"
#include "Icons.h"
#include "Fonts.h"
#include "Config.h"


using SmoothText::LEFT;
using SmoothText::CENTER;
using SmoothText::RIGHT;

static const uint16_t CHIP_COLOR = ROW_ALT_COLOR; // Tło "chipów" z podpowiedziami

// Podpis gładką czcionką (statyczny)
static void label(TFT_eSPI& tft, const char* txt, int x, int y, int w, SmoothText::Align a = LEFT,
                  uint16_t bg = CARD_COLOR) {
    SmoothText::draw(tft, FontLabel, txt, x, y, w, 14, a, TEXT_MUTED, bg);
}

// Karta: zaokrąglone tło + ikona + podpis
static void card(TFT_eSPI& tft, int x, int y, const char* title) {
    tft.fillRoundRect(x, y, 148, 88, CARD_RADIUS, CARD_COLOR);
    label(tft, title, x + 32, y + 11, 112);
}

// Rysuje statyczny układ 4 kart z ikonami (raz przy wejściu na ekran)
void MotoStatsScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    fSpeed.reset(); fRpm.reset(); fTemp.reset(); fVolt.reset(); fDtc.reset(); fDtcNote.reset();
    tft.fillScreen(BG_COLOR);
    label(tft, "STATYSTYKI", 10, 3, 80, LEFT, BG_COLOR);

    card(tft, 8, 22, "Prędkość maks.");
    Icons::speedometer(tft, 16, 30, ACCENT_BLUE, CARD_COLOR);
    label(tft, "obroty", 16, 91, 60);

    card(tft, 164, 22, "Temperatura maks.");
    Icons::thermometer(tft, 172, 30, ACCENT_ORANGE, CARD_COLOR);
    label(tft, "od ostatniego zerowania", 172, 91, 136);

    card(tft, 8, 118, "Napięcie min.");
    Icons::battery(tft, 16, 126, ACCENT_GREEN, CARD_COLOR);
    label(tft, "przy rozruchu", 16, 187, 136);

    card(tft, 164, 118, "Błędy silnika");
    Icons::warning(tft, 172, 126, ACCENT_YELLOW, CARD_COLOR);

    // Podpowiedzi przycisków jako zaokrąglone "chipy"
    tft.fillRoundRect(8, 214, 148, 22, 11, CHIP_COLOR);
    label(tft, "MODE 2 s - zeruj rekordy", 20, 218, 124, CENTER, CHIP_COLOR);
    tft.fillRoundRect(164, 214, 148, 22, 11, CHIP_COLOR);
    label(tft, "ACTION 2 s - kasuj błędy", 176, 218, 124, CENTER, CHIP_COLOR);
}

// Odświeża wartości (każda rysuje się tylko po zmianie)
void MotoStatsScreen::update(TFT_eSPI& tft, const MotoState& state) {
    fSpeed.show(tft, FontNumXL, String(state.maxSpeedKmh), 16, 52, 136, 36, LEFT, TFT_WHITE, CARD_COLOR);
    fRpm.show(tft, FontNumM, String(state.maxRpm), 76, 89, 72, 18, RIGHT, TFT_WHITE, CARD_COLOR);

    fTemp.show(tft, FontNumXL, String(state.maxEngineTempC) + "\xC2\xB0", 172, 52, 136, 36, LEFT,
               state.maxEngineTempC > Config::ENGINE_TEMP_WARN ? ACCENT_RED : TFT_WHITE, CARD_COLOR);

    fVolt.show(tft, FontNumXL, String(state.minBootVoltageV, 1), 16, 148, 136, 36, LEFT,
               state.minBootVoltageV < 10.5f ? ACCENT_RED : TFT_WHITE, CARD_COLOR);

    bool hasDtc = state.dtcCount > 0;
    fDtc.show(tft, FontNumXL, String(state.dtcCount), 172, 148, 136, 36, LEFT,
              hasDtc ? ACCENT_RED : ACCENT_GREEN, CARD_COLOR);
    fDtcNote.show(tft, FontLabel, hasDtc ? "do sprawdzenia" : "brak błędów", 172, 187, 136, 14, LEFT,
                  TEXT_MUTED, CARD_COLOR);
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
