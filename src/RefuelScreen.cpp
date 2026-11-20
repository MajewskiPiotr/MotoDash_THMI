#include "RefuelScreen.h"

// Paleta kolorów w formacie RGB565 (spójna z pozostałymi ekranami)
#define BG_COLOR      0x0841 // Ciemny grafit (tło główne)
#define CARD_COLOR    0x10A3 // Ciemnoszary (wnętrze kafelków)
#define HIGHLIGHT_BG  0x1B4F // Granatowy (tło zaznaczonej opcji)
#define BORDER_COLOR  0x2965 // Szary (obramowania kafelków)
#define TEXT_MUTED    0x8C71 // Przygaszony jasnoszary (etykiety)
#define ACCENT_GREEN  0x07E0 // Zielony (pełny bak, "DO PELNA")
#define ACCENT_BLUE   0x05FF // Błękitny (nagłówek, zasięg, "DOLEJ")
#define ACCENT_YELLOW 0xFFE0 // Żółty (rezerwa)
#define ACCENT_RED    0xF800 // Czerwony (prawie pusty bak)
#define BAR_PREVIEW   0x0320 // Ciemnozielony (podgląd stanu po dolaniu na pasku)

// --- UKŁAD EKRANU 320x240 ---
// Karta stanu paliwa
static constexpr int FUEL_X = 8,   FUEL_Y = 22,  FUEL_W = 304, FUEL_H = 72;
static constexpr int BAR_X  = 16,  BAR_Y  = 78,  BAR_W  = 288, BAR_H  = 10;
// Wybór ilości: [-]  +5.0 L  [+]
static constexpr int AMT_Y  = 100, AMT_H  = 62;
static constexpr int MINUS_X = 8,  PLUS_X = 248, PM_W = 64;
// Dolny rząd przycisków
static constexpr int BTN_Y  = 168, BTN_H  = 64,  BTN_W  = 96;
static constexpr int FULL_X = 8,   ADD_X  = 112, CANCEL_X = 216;

// --- PARAMETRY WYBORU ILOŚCI ---
static const float AMOUNT_STEP    = 0.5f; // Krok zmiany ilości dolewanego paliwa [L]
static const float DEFAULT_AMOUNT = 5.0f; // Domyślna ilość po otwarciu ekranu [L]

// ---------------------------------------------------------------------------
// Otwieranie / zamykanie
// ---------------------------------------------------------------------------

void RefuelScreen::open(MotoState& state) {
    // Domyślnie proponujemy 5 L, ale nie więcej niż zmieści się w baku
    amountL  = min(DEFAULT_AMOUNT, maxAmountL(state));
    selected = ITEM_PLUS;
    if (screenManager) screenManager->showOverlay(this, state);
    Serial.println("[Refuel] Otwarto ekran tankowania");
}

void RefuelScreen::close(MotoState& state) {
    if (screenManager) screenManager->closeOverlay(state);
}

// ---------------------------------------------------------------------------
// Logika ilości
// ---------------------------------------------------------------------------

float RefuelScreen::freeSpaceL(const MotoState& state) const {
    return max(0.0f, state.tankCapacityL - state.fuelInTankL);
}

float RefuelScreen::maxAmountL(const MotoState& state) const {
    // Zaokrąglamy w dół do pełnego kroku (np. 7.3 L wolnego -> max 7.0 L)
    return floorf(freeSpaceL(state) / AMOUNT_STEP) * AMOUNT_STEP;
}

void RefuelScreen::changeAmount(const MotoState& state, int steps) {
    float maxL = maxAmountL(state);
    amountL = constrain(amountL + steps * AMOUNT_STEP, min(AMOUNT_STEP, maxL), maxL);
    controlsDirty = true;
}

void RefuelScreen::activate(MotoState& state, uint8_t item) {
    switch (item) {
        case ITEM_MINUS:
            changeAmount(state, -1);
            break;
        case ITEM_PLUS:
            changeAmount(state, +1);
            break;
        case ITEM_FULL:
            if (fuelCalculator) fuelCalculator->resetRefuel(state);
            close(state);
            break;
        case ITEM_ADD:
            if (amountL <= 0.0f) break; // Bak pełny - nic do dolania
            if (fuelCalculator) fuelCalculator->addFuel(state, amountL);
            close(state);
            break;
        case ITEM_CANCEL:
            close(state);
            break;
    }
}

// ---------------------------------------------------------------------------
// Obsługa wejścia
// ---------------------------------------------------------------------------

bool RefuelScreen::onModeClick(MotoState& state) {
    selected = (selected + 1) % ITEM_COUNT; // Następna opcja
    controlsDirty = true;
    return true;
}

bool RefuelScreen::onModeLongPress(MotoState& state) {
    close(state); // Anuluj i wróć
    return true;
}

bool RefuelScreen::onActionClick(MotoState& state) {
    activate(state, selected); // Wykonaj zaznaczoną opcję
    return true;               // Blokuje globalną zmianę jasności
}

bool RefuelScreen::onActionLongPress(MotoState& state) {
    activate(state, ITEM_FULL); // Skrót: do pełna
    return true;
}

// ---------------------------------------------------------------------------
// Rysowanie
// ---------------------------------------------------------------------------

void RefuelScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    controlsDirty  = true;
    lastDrawnFuelL = -1.0f;
    tft.fillScreen(BG_COLOR);

    tft.setTextColor(ACCENT_BLUE, BG_COLOR);
    tft.drawString("TANKOWANIE", 10, 6, 1);
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    tft.drawRightString("[MODE]: Wybor  [ACT]: OK", 310, 6, 1);

    // Karta stanu paliwa
    tft.fillRoundRect(FUEL_X, FUEL_Y, FUEL_W, FUEL_H, 7, CARD_COLOR);
    tft.drawRoundRect(FUEL_X, FUEL_Y, FUEL_W, FUEL_H, 7, BORDER_COLOR);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.drawString("STAN PALIWA W BAKU", 16, 30, 1);
    tft.drawRightString("ZASIEG", 304, 30, 1);

    // Karta wyboru ilości (środek między przyciskami - i +)
    tft.fillRoundRect(MINUS_X + PM_W + 6, AMT_Y, PLUS_X - MINUS_X - PM_W - 12, AMT_H, 7, CARD_COLOR);
    tft.drawRoundRect(MINUS_X + PM_W + 6, AMT_Y, PLUS_X - MINUS_X - PM_W - 12, AMT_H, 7, BORDER_COLOR);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.drawCentreString("ILOSC DO DOLANIA", 160, AMT_Y + 6, 1);
}

void RefuelScreen::update(TFT_eSPI& tft, const MotoState& state) {
    // Karta paliwa: odświeżamy po zmianie wartości, ale nie częściej niż co 500 ms
    unsigned long now = millis();
    if (controlsDirty || (now - lastFuelDrawMs > 500 && fabsf(state.fuelInTankL - lastDrawnFuelL) >= 0.05f)) {
        lastFuelDrawMs = now;
        drawFuelCard(tft, state);
    }

    if (controlsDirty) {
        controlsDirty = false;
        // Ilość nie może przekroczyć wolnego miejsca (np. jeśli w międzyczasie zmieniła się pojemność)
        amountL = min(amountL, maxAmountL(state));
        drawControls(tft, state);
    }
}

void RefuelScreen::drawFuelCard(TFT_eSPI& tft, const MotoState& state) {
    lastDrawnFuelL = state.fuelInTankL;
    float capacity = max(1.0f, state.tankCapacityL);
    float ratio    = constrain(state.fuelInTankL / capacity, 0.0f, 1.0f);
    float afterRatio = constrain((state.fuelInTankL + amountL) / capacity, 0.0f, 1.0f);

    uint16_t levelColor = (ratio < 0.10f) ? ACCENT_RED
                        : (ratio < 0.25f) ? ACCENT_YELLOW
                        : ACCENT_GREEN;

    // Duża liczba litrów + pojemność baku
    tft.setTextColor(levelColor, CARD_COLOR);
    tft.setTextPadding(110);
    String litres = String(state.fuelInTankL, 1) + " L";
    tft.drawString(litres, 16, 44, 4);
    tft.setTextPadding(0);

    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.setTextPadding(80);
    tft.drawString("/ " + String(state.tankCapacityL, 1) + " L  (" + String((int)(ratio * 100)) + "%)", 128, 52, 2);
    tft.setTextPadding(0);

    // Zasięg po prawej
    tft.setTextColor(ACCENT_BLUE, CARD_COLOR);
    tft.setTextPadding(60);
    tft.drawRightString("~" + String(state.rangeKm) + " km", 304, 52, 2);
    tft.setTextPadding(0);

    // Pasek poziomu paliwa: obecny stan + podgląd stanu po dolaniu wybranej ilości
    int fillW  = (int)(BAR_W * ratio);
    int afterW = (int)(BAR_W * afterRatio);
    tft.fillRect(BAR_X, BAR_Y, BAR_W, BAR_H, BG_COLOR);
    if (afterW > fillW) tft.fillRect(BAR_X + fillW, BAR_Y, afterW - fillW, BAR_H, BAR_PREVIEW);
    if (fillW > 0)      tft.fillRect(BAR_X, BAR_Y, fillW, BAR_H, levelColor);
    tft.drawRect(BAR_X - 1, BAR_Y - 1, BAR_W + 2, BAR_H + 2, BORDER_COLOR);
}

void RefuelScreen::drawControls(TFT_eSPI& tft, const MotoState& state) {
    bool tankFull = maxAmountL(state) <= 0.0f;

    // Przyciski - / +
    drawButton(tft, MINUS_X, AMT_Y, PM_W, AMT_H, "-", TFT_WHITE, selected == ITEM_MINUS, 4);
    drawButton(tft, PLUS_X,  AMT_Y, PM_W, AMT_H, "+", TFT_WHITE, selected == ITEM_PLUS,  4);

    // Wybrana ilość oraz stan baku po dolaniu
    tft.setTextColor(tankFull ? TEXT_MUTED : TFT_WHITE, CARD_COLOR);
    tft.setTextPadding(150);
    tft.drawCentreString(tankFull ? String("BAK PELNY") : String("+" + String(amountL, 1) + " L"), 160, AMT_Y + 18, 4);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.drawCentreString("po: " + String(state.fuelInTankL + amountL, 1) + " L", 160, AMT_Y + 46, 1);
    tft.setTextPadding(0);

    // Dolny rząd: DO PELNA / DOLEJ / ANULUJ
    drawButton(tft, FULL_X,   BTN_Y, BTN_W, BTN_H, "DO PELNA", ACCENT_GREEN, selected == ITEM_FULL, 2);
    drawButton(tft, ADD_X,    BTN_Y, BTN_W, BTN_H, "DOLEJ",    tankFull ? TEXT_MUTED : ACCENT_BLUE,
               selected == ITEM_ADD, 2);
    drawButton(tft, CANCEL_X, BTN_Y, BTN_W, BTN_H, "ANULUJ",   TEXT_MUTED, selected == ITEM_CANCEL, 2);
}

void RefuelScreen::drawButton(TFT_eSPI& tft, int x, int y, int w, int h, const String& label,
                              uint16_t accent, bool isSelected, uint8_t font) {
    uint16_t bg = isSelected ? HIGHLIGHT_BG : CARD_COLOR;
    tft.fillRoundRect(x, y, w, h, 7, bg);
    tft.drawRoundRect(x, y, w, h, 7, isSelected ? TFT_WHITE : accent);
    if (isSelected) tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 6, TFT_WHITE); // Grubsza ramka zaznaczenia

    int fontH = (font == 4) ? 26 : 16;
    tft.setTextColor(isSelected ? TFT_WHITE : accent, bg);
    tft.drawCentreString(label, x + w / 2, y + (h - fontH) / 2, font);
}
