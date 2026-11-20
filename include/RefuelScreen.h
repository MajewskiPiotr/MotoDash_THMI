#pragma once
#include "Screen.h"
#include "FuelCalculator.h"
#include "ScreenManager.h"

// Ekran-nakładka "TANKOWANIE" (otwierany podwójnym kliknięciem ACTION na ekranie jazdy).
// Pokazuje aktualny stan paliwa w baku i pozwala zatankować do pełna lub dolać konkretną ilość.
//
// Opcje: [-] / [+] zmienia ilość, [DO PELNA], [DOLEJ], [ANULUJ].
// Sterowanie przyciskami: MODE = następna opcja, ACTION = wykonaj zaznaczoną opcję,
//                         MODE 2s = anuluj, ACTION 2s = do pełna.
class RefuelScreen : public Screen {
public:
    RefuelScreen(FuelCalculator* fuelCalc, ScreenManager* mgr)
        : fuelCalculator(fuelCalc), screenManager(mgr) {}

    // Przygotowuje domyślną ilość i wyświetla ekran jako nakładkę.
    // autoOpened = true: otwarty automatycznie po starcie przy prawie pustym baku (zaznacza DO PELNA).
    void open(MotoState& state, bool autoOpened = false);

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;

    bool onModeClick(MotoState& state) override;
    bool onModeLongPress(MotoState& state) override;
    bool onActionClick(MotoState& state) override;
    bool onActionLongPress(MotoState& state) override;

private:
    // Opcje na ekranie (kolejność przechodzenia przyciskiem MODE)
    enum Item : uint8_t { ITEM_MINUS = 0, ITEM_PLUS, ITEM_FULL, ITEM_ADD, ITEM_CANCEL, ITEM_COUNT };

    FuelCalculator* fuelCalculator;
    ScreenManager*  screenManager;

    float   amountL      = 5.0f;           // Wybrana ilość do dolania [L]
    uint8_t selected     = ITEM_PLUS;      // Zaznaczona opcja (sterowanie przyciskami)
    bool    controlsDirty = true;          // Przerysuj przyciski i wybraną ilość
    unsigned long lastFuelDrawMs = 0;      // Ostatnie odświeżenie karty stanu paliwa
    float   lastDrawnFuelL = -1.0f;        // Ostatnio narysowana ilość paliwa (odświeżamy tylko po zmianie)
    MotoState*    statePtr       = nullptr; // Stan aplikacji (potrzebny do automatycznego zamknięcia w update())
    unsigned long lastActivityMs = 0;       // Czas ostatniego naciśnięcia przycisku na tym ekranie
    int           lastShownSec   = -1;      // Ostatnio narysowana liczba sekund do automatycznego powrotu

    float freeSpaceL(const MotoState& state) const; // Ile litrów zmieści się jeszcze w baku
    float maxAmountL(const MotoState& state) const; // Maksymalna ilość do wyboru (wolne miejsce w baku)
    bool  isTankFull(const MotoState& state) const; // Bak pełny -> -, + i DOLEJ są nieaktywne
    bool  isItemEnabled(const MotoState& state, uint8_t item) const;
    void  changeAmount(const MotoState& state, int steps);
    void  activate(MotoState& state, uint8_t item); // Wykonuje akcję wybranej opcji
    void  close(MotoState& state);
    void  touchActivity() { lastActivityMs = millis(); } // Resetuje licznik bezczynności

    void drawFuelCard(TFT_eSPI& tft, const MotoState& state);
    void drawControls(TFT_eSPI& tft, const MotoState& state);
    void drawButton(TFT_eSPI& tft, int x, int y, int w, int h, const String& label,
                    uint16_t accent, bool isSelected, uint8_t font);
};
