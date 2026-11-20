#include "SettingsScreen.h"
#include "Theme.h" // Wspólna paleta kolorów
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

// Rysuje statyczne tło, nagłówek oraz dolną instrukcję przycisków (raz przy wejściu do menu)
void SettingsScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    isDirty = true; // Wymusza narysowanie wszystkich wierszy przy pierwszym wywołaniu update()
    tft.fillScreen(BG_COLOR);

    tft.setTextColor(ACCENT_BLUE, BG_COLOR);
    tft.drawString("KONFIGURACJA SYSTEMU", 10, 6, 1);
    tft.drawRoundRect(8, 22, 304, 188, 7, BORDER_COLOR);

    // Dolna legenda podpowiadająca działanie przycisków w menu
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    tft.drawString("[MODE]: Wybor   [ACT]: +   [ACT 2s]: -", 10, 218, 1);
}

// Rysuje listę 6 wierszy z aktualnymi wartościami
void SettingsScreen::update(TFT_eSPI& tft, const MotoState& state) {
    // Rysujemy listę TYLKO wtedy, gdy użytkownik zmienił wiersz lub wartość (eliminuje migotanie)
    if (!isDirty) return;
    isDirty = false; // Resetuje flagę po narysowaniu aktualnego stanu

    // Etykiety wyświetlanych parametrów
    const char* labels[TOTAL_ROWS] = {
        "Kalibracja spalania:",
        "Pojemnosc baku:",
        "Alarm TPMS Przod:",
        "Alarm TPMS Tyl:",
        "Wysokosc startowa:",
        "ZAPISZ I WYJDZ"
    };

    // Aktualne wartości sformatowane jako tekst
    String values[TOTAL_ROWS] = {
        String(state.fuelCalFactor, 2),
        String(state.tankCapacityL, 1) + " L",
        String(state.minFrontBar, 1) + " bar",
        String(state.minRearBar, 1) + " bar",
        String(state.baseAltitudeM) + " m",
        "[OK]"
    };

    // Pętla rysująca każdy z 6 wierszy listy
    for (uint8_t i = 0; i < TOTAL_ROWS; i++) {
        int y = 28 + (i * 29); // Wylicza pozycję pionową (Y) kolejnego wiersza
        bool isSelected = (i == selectedRow);

        // Podświetla aktywny wiersz kolorem HIGHLIGHT_BG
        uint16_t rowBg = isSelected ? HIGHLIGHT_BG : CARD_COLOR;
        tft.fillRoundRect(12, y, 296, 25, 4, rowBg);

        // Rysuje nazwę parametru po lewej stronie ze strzałką '>' dla wybranej pozycji
        tft.setTextColor(isSelected ? TFT_WHITE : TEXT_MUTED, rowBg);
        String prefix = isSelected ? "> " : "  ";
        tft.drawString(prefix + String(labels[i]), 18, y + 5, 2);

        // Rysuje wartość parametru wyrównaną do prawej krawędzi
        tft.setTextColor(isSelected ? ACCENT_GREEN : TFT_WHITE, rowBg);
        tft.drawRightString(values[i], 300, y + 5, 2);
    }
}

// Krótkie kliknięcie MODE przesuwa kursor na kolejny wiersz w menu
bool SettingsScreen::onModeClick(MotoState& state) {
    selectedRow = (selectedRow + 1) % TOTAL_ROWS; // Przechodzi cyklicznie 0 -> 1 -> ... -> 5 -> 0
    isDirty = true;                               // Oznacza menu do przerysowania
    return true;                                  // Blokuje przełączenie na inny ekran przez ScreenManager
}

// Długie przytrzymanie MODE zapisuje ustawienia i wraca do Ekranu 1 (Tryb Jazdy)
bool SettingsScreen::onModeLongPress(MotoState& state) {
    saveToNvs(state);
    selectedRow = 0;
    if (screenManager) screenManager->goToFirstScreen(state);
    return true;
}

// Krótkie kliknięcie ACTION zwiększa wybraną wartość (+) lub zatwierdza wyjście
bool SettingsScreen::onActionClick(MotoState& state) {
    switch (selectedRow) {
        case 0: // Kalibracja spalania (krok co 0.02 w zakresie 0.70 .. 1.30, jak przy automatycznej kalibracji)
            state.fuelCalFactor += 0.02f;
            if (state.fuelCalFactor > 1.30f) state.fuelCalFactor = 0.70f;
            break;
        case 1: // Pojemność baku (krok co 0.5 L w zakresie 10.0 .. 30.0 L)
            state.tankCapacityL += 0.5f;
            if (state.tankCapacityL > 30.0f) state.tankCapacityL = 10.0f;
            break;
        case 2: // Próg alarmu przedniej opony (krok co 0.1 bar w zakresie 1.5 .. 2.8 bar)
            state.minFrontBar += 0.1f;
            if (state.minFrontBar > 2.8f) state.minFrontBar = 1.5f;
            break;
        case 3: // Próg alarmu tylnej opony (krok co 0.1 bar w zakresie 1.5 .. 3.2 bar)
            state.minRearBar += 0.1f;
            if (state.minRearBar > 3.2f) state.minRearBar = 1.5f;
            break;
        case 4: // Wysokość bazowa do kalibracji barometru (krok co 20 m w zakresie 0 .. 2500 m)
            state.baseAltitudeM += 20;
            if (state.baseAltitudeM > 2500) state.baseAltitudeM = 0;
            break;
        case 5: // Ostatnia pozycja: "ZAPISZ I WYJDZ"
            saveToNvs(state);
            selectedRow = 0;
            if (screenManager) screenManager->goToFirstScreen(state);
            return true;
    }
    isDirty = true; // Wymusza przerysowanie listy ze zaktualizowaną wartością
    return true;    // Blokuje globalną zmianę jasności ekranu
}

// Długie przytrzymanie ACTION zmniejsza wybraną wartość (-)
bool SettingsScreen::onActionLongPress(MotoState& state) {
    switch (selectedRow) {
        case 0:
            state.fuelCalFactor = max(0.70f, state.fuelCalFactor - 0.02f);
            break;
        case 1:
            state.tankCapacityL = max(10.0f, state.tankCapacityL - 0.5f);
            break;
        case 2:
            state.minFrontBar = max(1.5f, state.minFrontBar - 0.1f);
            break;
        case 3:
            state.minRearBar = max(1.5f, state.minRearBar - 0.1f);
            break;
        case 4:
            state.baseAltitudeM = max(0, state.baseAltitudeM - 20);
            break;
    }
    isDirty = true; // Wymusza przerysowanie listy ze zmniejszoną wartością
    return true;
}
