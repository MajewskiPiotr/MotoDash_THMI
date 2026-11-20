#include "RefuelScreen.h"
#include "Theme.h" // Wspólna paleta kolorów
#include "Config.h"


// --- UKŁAD EKRANU 320x240 ---
// Górna karta (stan paliwa / informacje o kalibracji)
static constexpr int FUEL_X = 8,   FUEL_Y = 22,  FUEL_W = 304, FUEL_H = 72;
static constexpr int BAR_X  = 16,  BAR_Y  = 78,  BAR_W  = 288, BAR_H  = 10;
// Wybór ilości: [-]  +5.0 L  [+]
static constexpr int AMT_Y  = 100, AMT_H  = 62;
static constexpr int MINUS_X = 8,  PLUS_X = 248, PM_W = 64;
// Dolny rząd przycisków
static constexpr int BTN_Y  = 168, BTN_H  = 64,  BTN_W  = 96;
static constexpr int FULL_X = 8,   ADD_X  = 112, CANCEL_X = 216;

// --- PARAMETRY ---
static const float AMOUNT_STEP    = 0.5f;  // Krok zmiany ilości dolewanego paliwa [L]
static const float DEFAULT_AMOUNT = 5.0f;  // Domyślna ilość po otwarciu ekranu [L]
static const float PUMP_STEP      = 0.1f;  // Krok zmiany litrów z dystrybutora przy kalibracji [L]
static const unsigned long RESULT_SHOW_MS = 6000; // Jak długo widać wynik kalibracji [ms]

// ---------------------------------------------------------------------------
// Otwieranie / zamykanie / przełączanie widoków
// ---------------------------------------------------------------------------

void RefuelScreen::open(MotoState& state, bool autoOpened) {
    statePtr = &state;
    view     = VIEW_MAIN;
    touchActivity(); // Start odliczania do automatycznego powrotu
    // Domyślnie proponujemy 5 L, ale nie więcej niż zmieści się w baku
    amountL  = min(DEFAULT_AMOUNT, maxAmountL(state));
    // Przy pełnym baku zaznaczamy ANULUJ, żeby przypadkowe ACTION niczego nie zmieniło
    if (isTankFull(state))  selected = ITEM_CANCEL;
    else if (autoOpened)    selected = ITEM_FULL;  // Po starcie z prawie pustym bakiem najczęściej tankujemy do pełna
    else                    selected = ITEM_PLUS;
    if (screenManager) screenManager->showOverlay(this, state);
    Serial.println(autoOpened ? "[Refuel] Malo paliwa po starcie - otwarto ekran tankowania"
                              : "[Refuel] Otwarto ekran tankowania");
}

void RefuelScreen::close(MotoState& state) {
    view = VIEW_MAIN;
    if (screenManager) screenManager->closeOverlay(state);
}

void RefuelScreen::switchView(MotoState& state, View v, uint8_t sel) {
    view     = v;
    selected = sel;
    touchActivity();
    state.forceRedraw = true; // ScreenManager narysuje nakładkę od nowa (onEnter dla nowego widoku)
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

bool RefuelScreen::isTankFull(const MotoState& state) const {
    return maxAmountL(state) < AMOUNT_STEP; // Nie zmieści się nawet 0.5 L
}

bool RefuelScreen::isItemEnabled(const MotoState& state, uint8_t item) const {
    if (view != VIEW_MAIN || !isTankFull(state)) return true;
    return item == ITEM_FULL || item == ITEM_CANCEL; // Przy pełnym baku tylko DO PELNA i ANULUJ
}

void RefuelScreen::changeAmount(const MotoState& state, int steps) {
    float maxL = maxAmountL(state);
    amountL = constrain(amountL + steps * AMOUNT_STEP, min(AMOUNT_STEP, maxL), maxL);
    controlsDirty = true;
}

void RefuelScreen::changePumped(const MotoState& state, int steps) {
    float maxL = max(1.0f, state.tankCapacityL * 1.5f); // Zapas na wypadek, gdyby komputer bardzo zaniżał
    pumpedL = constrain(pumpedL + steps * PUMP_STEP, PUMP_STEP, maxL);
    pumpedL = roundf(pumpedL * 10.0f) / 10.0f;          // Bez błędów zaokrągleń typu 14.999
    controlsDirty = true;
}

// ---------------------------------------------------------------------------
// Akcje
// ---------------------------------------------------------------------------

void RefuelScreen::activate(MotoState& state, uint8_t item) {
    if (!isItemEnabled(state, item)) return; // Nieaktywny przycisk (np. + przy pełnym baku)
    switch (item) {
        case ITEM_MINUS:  changeAmount(state, -1); break;
        case ITEM_PLUS:   changeAmount(state, +1); break;
        case ITEM_FULL:   startFullRefuel(state);  break;
        case ITEM_ADD:
            if (fuelCalculator) fuelCalculator->addFuel(state, amountL);
            close(state);
            break;
        case ITEM_CANCEL: close(state); break;
    }
}

void RefuelScreen::activateCalib(MotoState& state, uint8_t item) {
    switch (item) {
        case ITEM_MINUS:  changePumped(state, -1);                     break;
        case ITEM_PLUS:   changePumped(state, +1);                     break;
        case ITEM_FULL:   finishCalibration(state, true);              break; // ZAPISZ
        case ITEM_ADD:    finishCalibration(state, false);             break; // POMIN
        case ITEM_CANCEL: switchView(state, VIEW_MAIN, ITEM_FULL);     break; // WSTECZ
    }
}

// "DO PELNA": jeśli są dane do kalibracji, pytamy o litry z dystrybutora; inaczej od razu pełny bak
void RefuelScreen::startFullRefuel(MotoState& state) {
    if (!fuelCalculator) { close(state); return; }

    if (fuelCalculator->canCalibrate()) {
        // Podpowiadamy tyle, ile wg komputera powinno wejść - zwykle trzeba poprawić tylko o kilka kroków
        pumpedL = max(PUMP_STEP, roundf(fuelCalculator->expectedRefillL() * 10.0f) / 10.0f);
        switchView(state, VIEW_CALIB, ITEM_PLUS);
        Serial.println("[Refuel] Kalibracja: podaj litry z dystrybutora");
    } else {
        fuelCalculator->resetRefuel(state);
        close(state);
    }
}

void RefuelScreen::finishCalibration(MotoState& state, bool save) {
    calApplied = false;
    calOldF = calNewF = state.fuelCalFactor;
    if (fuelCalculator) {
        // Kalibracja MUSI być przed resetRefuel() - korzysta z licznika spalonego paliwa
        if (save) calApplied = fuelCalculator->calibrateWithPump(state, pumpedL, calOldF, calNewF);
        fuelCalculator->resetRefuel(state);
    }
    if (save) switchView(state, VIEW_RESULT, 0); // Pokaż wynik kalibracji
    else      close(state);
}

// ---------------------------------------------------------------------------
// Obsługa przycisków
// ---------------------------------------------------------------------------

bool RefuelScreen::onModeClick(MotoState& state) {
    touchActivity();
    if (view == VIEW_RESULT) { close(state); return true; }
    // Następna aktywna opcja (przy pełnym baku w widoku głównym pomija -, + i DOLEJ)
    do {
        selected = (selected + 1) % ITEM_COUNT;
    } while (!isItemEnabled(state, selected));
    controlsDirty = true;
    return true;
}

bool RefuelScreen::onModeLongPress(MotoState& state) {
    touchActivity();
    if (view == VIEW_CALIB) switchView(state, VIEW_MAIN, ITEM_FULL); // Wstecz
    else                    close(state);                            // Anuluj i wróć
    return true;
}

bool RefuelScreen::onActionClick(MotoState& state) {
    touchActivity();
    if      (view == VIEW_RESULT) close(state);
    else if (view == VIEW_CALIB)  activateCalib(state, selected);
    else                          activate(state, selected);
    return true; // Blokuje globalną zmianę jasności
}

bool RefuelScreen::onActionLongPress(MotoState& state) {
    touchActivity();
    if      (view == VIEW_RESULT) close(state);
    else if (view == VIEW_CALIB)  activateCalib(state, ITEM_FULL); // Skrót: zapisz kalibrację
    else                          activate(state, ITEM_FULL);      // Skrót: do pełna
    return true;
}

// ---------------------------------------------------------------------------
// Rysowanie
// ---------------------------------------------------------------------------

void RefuelScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    controlsDirty  = true;
    lastDrawnFuelL = -1.0f;
    lastShownSec   = -1;
    tft.fillScreen(BG_COLOR);

    if (view == VIEW_RESULT) {
        drawHeader(tft, "KALIBRACJA SPALANIA");
        return; // Treść rysuje drawResult() w update()
    }

    drawHeader(tft, view == VIEW_CALIB ? "KALIBRACJA SPALANIA" : "TANKOWANIE");

    // Górna karta
    tft.fillRoundRect(FUEL_X, FUEL_Y, FUEL_W, FUEL_H, 7, CARD_COLOR);
    tft.drawRoundRect(FUEL_X, FUEL_Y, FUEL_W, FUEL_H, 7, BORDER_COLOR);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    if (view == VIEW_MAIN) {
        tft.drawString("STAN PALIWA W BAKU", 16, 30, 1);
        tft.drawRightString("ZASIEG", 304, 30, 1);
    } else {
        tft.drawString("WPISZ LITRY Z DYSTRYBUTORA", 16, 30, 1);
    }

    // Karta wyboru ilości (środek między przyciskami - i +)
    tft.fillRoundRect(MINUS_X + PM_W + 6, AMT_Y, PLUS_X - MINUS_X - PM_W - 12, AMT_H, 7, CARD_COLOR);
    tft.drawRoundRect(MINUS_X + PM_W + 6, AMT_Y, PLUS_X - MINUS_X - PM_W - 12, AMT_H, 7, BORDER_COLOR);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.drawCentreString(view == VIEW_MAIN ? "ILOSC DO DOLANIA" : "WLANO DO PELNA", 160, AMT_Y + 6, 1);
}

void RefuelScreen::update(TFT_eSPI& tft, const MotoState& state) {
    // Automatyczne zamknięcie po czasie bez naciśnięcia przycisku
    unsigned long timeout = (view == VIEW_RESULT) ? RESULT_SHOW_MS : Config::REFUEL_TIMEOUT_MS;
    if (millis() - lastActivityMs >= timeout) {
        if (!statePtr) return;
        if (view == VIEW_CALIB) {
            // Użytkownik wybrał już "DO PELNA" - tankujemy do pełna, tylko bez kalibracji
            Serial.println("[Refuel] Brak reakcji w kalibracji - do pelna bez kalibracji");
            if (fuelCalculator) fuelCalculator->resetRefuel(*statePtr);
        } else {
            Serial.println("[Refuel] Brak reakcji - powrot do ekranu jazdy");
        }
        close(*statePtr);
        return;
    }

    if (view == VIEW_RESULT) {
        if (controlsDirty) { controlsDirty = false; drawResult(tft, state); }
        return;
    }

    drawCountdown(tft, timeout);

    if (view == VIEW_CALIB) {
        if (controlsDirty) {
            controlsDirty = false;
            drawCalibInfo(tft, state);
            drawCalibControls(tft, state);
        }
        return;
    }

    // Widok główny. Karta paliwa: odświeżamy po zmianie wartości, ale nie częściej niż co 500 ms
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

void RefuelScreen::drawHeader(TFT_eSPI& tft, const char* title) {
    tft.setTextColor(ACCENT_BLUE, BG_COLOR);
    tft.drawString(title, 10, 6, 1);
    if (view != VIEW_RESULT) {
        tft.setTextColor(TEXT_MUTED, BG_COLOR);
        tft.drawRightString("[MODE]: Wybor  [ACT]: OK", 310, 6, 1);
    }
}

void RefuelScreen::drawCountdown(TFT_eSPI& tft, unsigned long timeoutMs) {
    unsigned long idleMs = millis() - lastActivityMs;
    int secLeft = (int)((timeoutMs - idleMs + 999) / 1000);
    if (secLeft == lastShownSec) return;
    lastShownSec = secLeft;
    // W widoku kalibracji tytuł jest dłuższy, więc licznik stoi dalej w prawo
    int x = (view == VIEW_CALIB) ? 128 : 80;
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    tft.setTextPadding(36);
    tft.drawString(String(secLeft) + " s", x, 6, 1);
    tft.setTextPadding(0);
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
    tft.drawString(String(state.fuelInTankL, 1) + " L", 16, 44, 4);
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
    bool  tankFull = isTankFull(state);
    float afterL   = min(state.tankCapacityL, state.fuelInTankL + amountL); // Stan po dolaniu
    uint16_t pmColor = tankFull ? BORDER_COLOR : TFT_WHITE; // Wyszarzone - i + przy pełnym baku

    // Przyciski - / +
    drawButton(tft, MINUS_X, AMT_Y, PM_W, AMT_H, "-", pmColor, selected == ITEM_MINUS, 4);
    drawButton(tft, PLUS_X,  AMT_Y, PM_W, AMT_H, "+", pmColor, selected == ITEM_PLUS,  4);

    // Wybrana ilość oraz stan baku po dolaniu
    tft.setTextPadding(150);
    if (tankFull) {
        tft.setTextColor(ACCENT_RED, CARD_COLOR);
        tft.drawCentreString("BAK PELNY", 160, AMT_Y + 18, 4);
        tft.setTextColor(TEXT_MUTED, CARD_COLOR);
        tft.drawCentreString("nie mozna dolac paliwa", 160, AMT_Y + 46, 1);
    } else {
        tft.setTextColor(TFT_WHITE, CARD_COLOR);
        tft.drawCentreString("+" + String(amountL, 1) + " L", 160, AMT_Y + 18, 4);
        tft.setTextColor(TEXT_MUTED, CARD_COLOR);
        tft.drawCentreString("po: " + String(afterL, 1) + " L", 160, AMT_Y + 46, 1);
    }
    tft.setTextPadding(0);

    // Dolny rząd: DO PELNA / DOLEJ / ANULUJ
    drawButton(tft, FULL_X,   BTN_Y, BTN_W, BTN_H, "DO PELNA", ACCENT_GREEN, selected == ITEM_FULL, 2);
    drawButton(tft, ADD_X,    BTN_Y, BTN_W, BTN_H, "DOLEJ",    tankFull ? BORDER_COLOR : ACCENT_BLUE,
               selected == ITEM_ADD, 2);
    drawButton(tft, CANCEL_X, BTN_Y, BTN_W, BTN_H, "ANULUJ",   TEXT_MUTED, selected == ITEM_CANCEL, 2);
}

void RefuelScreen::drawCalibInfo(TFT_eSPI& tft, const MotoState& state) {
    float expected = fuelCalculator ? fuelCalculator->expectedRefillL() : 0.0f;
    tft.setTextPadding(280);
    tft.setTextColor(TFT_WHITE, CARD_COLOR);
    tft.drawString("Komputer wyliczyl: " + String(expected, 1) + " L", 16, 46, 2);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    tft.drawString("Obecny mnoznik spalania: " + String(state.fuelCalFactor, 2), 16, 68, 2);
    tft.setTextPadding(0);
}

void RefuelScreen::drawCalibControls(TFT_eSPI& tft, const MotoState& state) {
    drawButton(tft, MINUS_X, AMT_Y, PM_W, AMT_H, "-", TFT_WHITE, selected == ITEM_MINUS, 4);
    drawButton(tft, PLUS_X,  AMT_Y, PM_W, AMT_H, "+", TFT_WHITE, selected == ITEM_PLUS,  4);

    // Wpisane litry i różnica względem wyliczenia komputera
    float expected = fuelCalculator ? fuelCalculator->expectedRefillL() : 0.0f;
    tft.setTextPadding(150);
    tft.setTextColor(TFT_WHITE, CARD_COLOR);
    tft.drawCentreString(String(pumpedL, 1) + " L", 160, AMT_Y + 18, 4);
    tft.setTextColor(TEXT_MUTED, CARD_COLOR);
    if (expected > 0.1f) {
        float diffPct = (pumpedL - expected) / expected * 100.0f;
        tft.drawCentreString("roznica: " + String(diffPct > 0 ? "+" : "") + String(diffPct, 1) + "%",
                             160, AMT_Y + 46, 1);
    } else {
        tft.drawCentreString(" ", 160, AMT_Y + 46, 1);
    }
    tft.setTextPadding(0);

    // Dolny rząd: ZAPISZ / POMIN / WSTECZ
    drawButton(tft, FULL_X,   BTN_Y, BTN_W, BTN_H, "ZAPISZ", ACCENT_GREEN, selected == ITEM_FULL,   2);
    drawButton(tft, ADD_X,    BTN_Y, BTN_W, BTN_H, "POMIN",  ACCENT_BLUE,  selected == ITEM_ADD,    2);
    drawButton(tft, CANCEL_X, BTN_Y, BTN_W, BTN_H, "WSTECZ", TEXT_MUTED,   selected == ITEM_CANCEL, 2);
}

void RefuelScreen::drawResult(TFT_eSPI& tft, const MotoState& state) {
    tft.fillRoundRect(8, 40, 304, 150, 7, CARD_COLOR);
    tft.drawRoundRect(8, 40, 304, 150, 7, calApplied ? ACCENT_GREEN : ACCENT_YELLOW);

    tft.setTextColor(ACCENT_GREEN, CARD_COLOR);
    tft.drawCentreString("ZATANKOWANO DO PELNA", 160, 52, 2);

    if (calApplied) {
        tft.setTextColor(TEXT_MUTED, CARD_COLOR);
        tft.drawCentreString("Mnoznik spalania", 160, 84, 2);
        tft.setTextColor(TFT_WHITE, CARD_COLOR);
        tft.drawCentreString(String(calOldF, 2) + "  ->  " + String(calNewF, 2), 160, 108, 4);
        tft.setTextColor(TEXT_MUTED, CARD_COLOR);
        tft.drawCentreString("Zapisano w ustawieniach", 160, 150, 2);
    } else {
        tft.setTextColor(ACCENT_YELLOW, CARD_COLOR);
        tft.drawCentreString("Mnoznika NIE zmieniono", 160, 90, 2);
        tft.setTextColor(TEXT_MUTED, CARD_COLOR);
        tft.drawCentreString("Roznica wieksza niz 30%.", 160, 118, 2);
        tft.drawCentreString("Sprawdz litry lub czy bak byl pelny.", 160, 140, 2);
    }

    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    tft.drawCentreString("Dowolny przycisk: OK", 160, 210, 2);
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
