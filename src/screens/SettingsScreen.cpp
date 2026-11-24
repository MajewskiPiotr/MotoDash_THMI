#include "SettingsScreen.h"
#include "Theme.h"
#include "SmoothText.h"
#include "Icons.h" // Wspólna paleta kolorów
#include "TouchHandler.h"
#include <Preferences.h> // Wbudowana biblioteka ESP32 do obsługi nieulotnej pamięci Flash (NVS)


// Wczytuje parametry konfiguracji z pamięci nieulotnej przy starcie systemu
void SettingsScreen::loadFromNvs(MotoState& state) {
    Preferences prefs;
    // Otwiera przestrzeń nazw "motodash" w trybie tylko do odczytu (true)
    if (prefs.begin("motodash", true)) {
        // Pobiera wartości spod kluczy; drugi argument to wartość domyślna przy pierwszym uruchomieniu
        state.fuelCalFactor = prefs.getFloat("fuelCal", 1.00f);
        state.tankCapacityL = prefs.getFloat("tankCap", 20.0f);
        state.minFrontBar   = prefs.getFloat("minFBar", 2.0f);
        state.minRearBar    = prefs.getFloat("minRBar", 2.2f);
        state.baseAltitudeM = prefs.getInt("baseAlt", 100);
        prefs.end(); // Zamyka dostęp do pamięci Flash
    }
}

// Zapisuje aktualne parametry z obiektu MotoState do pamięci nieulotnej
void SettingsScreen::saveToNvs(const MotoState& state) {
    Preferences prefs;
    // Otwiera przestrzeń nazw "motodash" w trybie zapisu (false)
    if (prefs.begin("motodash", false)) {
        prefs.putFloat("fuelCal", state.fuelCalFactor);
        prefs.putFloat("tankCap", state.tankCapacityL);
        prefs.putFloat("minFBar", state.minFrontBar);
        prefs.putFloat("minRBar", state.minRearBar);
        prefs.putInt("baseAlt", state.baseAltitudeM);
        prefs.end(); // Zatwierdza zapis w sektorach Flash
        Serial.println("[Settings] Zapisano konfiguracje do pamieci Flash!");
    }
}

// --- UKŁAD (dotyk) ---
// Wiersze parametrów: stuknięcie zaznacza wiersz. Na dole duże przyciski: [-] [+] [GOTOWE].
static constexpr int ROW_Y0 = 24, ROW_H = 24, ROW_STEP = 26;
static const TouchRect BTN_MINUS = { 8,   184, 96, 52 };
static const TouchRect BTN_PLUS  = { 112, 184, 96, 52 };
static const TouchRect BTN_DONE  = { 216, 184, 96, 52 };

void SettingsScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    isDirty = true;
    tft.fillScreen(BG_COLOR);
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    SmoothText::str(tft, "USTAWIENIA", 10, 6, 1);
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    SmoothText::str(tft, "stuknij wiersz", 230, 6, 1, SmoothText::RIGHT);
}

static void drawBtn(TFT_eSPI& tft, const TouchRect& r, const char* txt, uint16_t accent, uint8_t font) {
    tft.fillRoundRect(r.x, r.y, r.w, r.h, CARD_RADIUS, CARD_COLOR);
    tft.drawRoundRect(r.x, r.y, r.w, r.h, CARD_RADIUS, accent);
    tft.setTextColor(accent, CARD_COLOR);
    int fh = SmoothText::height(tft, font);
    SmoothText::str(tft, txt, r.x + r.w / 2, r.y + (r.h - fh) / 2, font, SmoothText::CENTER);
}

// Rysuje listę parametrów i dolne przyciski (tylko po zmianie)
void SettingsScreen::update(TFT_eSPI& tft, const MotoState& state) {
    if (!isDirty) return;
    isDirty = false;

    const char* labels[TOTAL_ROWS] = {
        "Kalibracja spalania:",
        "Pojemność baku:",
        "Alarm TPMS przód:",
        "Alarm TPMS tył:",
        "Wysokość startowa:",
        "Kalibracja dotyku"
    };
    String values[TOTAL_ROWS] = {
        String(state.fuelCalFactor, 2),
        String(state.tankCapacityL, 1) + " L",
        String(state.minFrontBar, 1) + " bar",
        String(state.minRearBar, 1) + " bar",
        String(state.baseAltitudeM) + " m",
        "stuknij"
    };

    for (uint8_t i = 0; i < TOTAL_ROWS; i++) {
        int y = ROW_Y0 + i * ROW_STEP;
        bool sel = (i == selectedRow);
        uint16_t rowBg = sel ? HIGHLIGHT_BG : CARD_COLOR;
        tft.fillRoundRect(8, y, 304, ROW_H, 8, rowBg);

        uint16_t ic = sel ? TFT_WHITE : TEXT_MUTED;
        switch (i) {
            case 0: Icons::droplet(tft, 14, y + 2, sel ? TFT_WHITE : ACCENT_BLUE); break;
            case 1: Icons::fuelPump(tft, 14, y + 2, sel ? TFT_WHITE : ACCENT_YELLOW, rowBg); break;
            case 2: case 3: Icons::tire(tft, 14, y + 1, ic, rowBg); break;
            case 4: Icons::mountain(tft, 14, y + 1, ic, rowBg); break;
            case 5: Icons::check(tft, 14, y + 1, ACCENT_GREEN); break;
        }
        tft.setTextColor(sel ? TFT_WHITE : TEXT_MUTED, rowBg);
        SmoothText::str(tft, labels[i], 42, y + 4, 2);
        tft.setTextColor(sel ? ACCENT_GREEN : TFT_WHITE, rowBg);
        SmoothText::str(tft, values[i], 300, y + 4, 2, SmoothText::RIGHT);
    }

    bool adjustable = selectedRow < ROW_TOUCH_CAL;
    drawBtn(tft, BTN_MINUS, "-", adjustable ? TFT_WHITE : BORDER_COLOR, 4);
    drawBtn(tft, BTN_PLUS,  "+", adjustable ? TFT_WHITE : BORDER_COLOR, 4);
    drawBtn(tft, BTN_DONE,  "GOTOWE", ACCENT_GREEN, 2);
}

// Zmienia wartość zaznaczonego wiersza o 'steps' kroków (ujemne = w dół) i od razu zapisuje
void SettingsScreen::changeValue(MotoState& state, int steps) {
    switch (selectedRow) {
        case 0: state.fuelCalFactor = constrain(state.fuelCalFactor + 0.02f * steps, 0.70f, 1.30f); break;
        case 1: state.tankCapacityL = constrain(state.tankCapacityL + 0.5f * steps, 10.0f, 30.0f); break;
        case 2: state.minFrontBar   = constrain(state.minFrontBar + 0.1f * steps, 1.5f, 2.8f); break;
        case 3: state.minRearBar    = constrain(state.minRearBar + 0.1f * steps, 1.5f, 3.2f); break;
        case 4: state.baseAltitudeM = constrain(state.baseAltitudeM + 20 * steps, 0, 2500); break;
        default: return;
    }
    saveToNvs(state);
    isDirty = true;
}

bool SettingsScreen::onTap(MotoState& state, int x, int y) {
    // Wiersz listy
    if (y >= ROW_Y0 && y < ROW_Y0 + TOTAL_ROWS * ROW_STEP && x >= 8 && x < 312) {
        uint8_t row = (y - ROW_Y0) / ROW_STEP;
        if (row == ROW_TOUCH_CAL) {
            if (touch && screenManager) {
                touch->calibrate(screenManager->display());
                state.forceRedraw = true; // Kalibracja zamalowała ekran
            }
            return true;
        }
        selectedRow = row;
        isDirty = true;
        return true;
    }
    if (BTN_MINUS.contains(x, y)) { changeValue(state, -1); return true; }
    if (BTN_PLUS.contains(x, y))  { changeValue(state, +1); return true; }
    if (BTN_DONE.contains(x, y)) {
        saveToNvs(state);
        if (screenManager) screenManager->goToFirstScreen(state);
        return true;
    }
    return false;
}

// Przytrzymanie - lub + zmienia wartość o 5 kroków naraz
bool SettingsScreen::onLongTouch(MotoState& state, int x, int y) {
    if (BTN_MINUS.contains(x, y)) { changeValue(state, -5); return true; }
    if (BTN_PLUS.contains(x, y))  { changeValue(state, +5); return true; }
    return false;
}
