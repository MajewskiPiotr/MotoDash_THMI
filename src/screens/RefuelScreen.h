#pragma once
#include "Screen.h"
#include "FuelCalculator.h"
#include "ScreenManager.h"
#include "RefuelLog.h"

// Ekran-nakładka "TANKOWANIE" (otwierany podwójnym kliknięciem ACTION na ekranie jazdy
// lub automatycznie po starcie przy prawie pustym baku).
//
// Widok główny:     [-] / [+] ilość dolewki, [DO PELNA], [DOLEJ], [ANULUJ]
// Widok kalibracji: po "DO PELNA" - wpisujesz litry z dystrybutora ([-] / [+] co 0.1 L),
//                   [ZAPISZ] koryguje mnożnik spalania, [POMIN] tankuje bez kalibracji, [WSTECZ] wraca.
// Widok wyniku:     pokazuje stary i nowy mnożnik, zamyka się sam po kilku sekundach.
//
// Przyciski: MODE = następna opcja, ACTION = wykonaj zaznaczoną opcję,
//            MODE 2s = anuluj / wstecz, ACTION 2s = do pełna (w kalibracji: zapisz).
class RefuelScreen : public Screen {
public:
    RefuelScreen(FuelCalculator* fuelCalc, ScreenManager* mgr, RefuelLog* log)
        : fuelCalculator(fuelCalc), screenManager(mgr), refuelLog(log) {}

    // Przygotowuje domyślną ilość i wyświetla ekran jako nakładkę.
    // autoOpened = true: otwarty automatycznie po starcie przy prawie pustym baku (zaznacza DO PELNA).
    void open(MotoState& state, bool autoOpened = false);

    // Szybkie "do pełna" bez ekranu (przytrzymanie ACTION na ekranie jazdy) - zapis w historii bez pomiaru
    void fullWithoutPump(MotoState& state);

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;

    bool onModeClick(MotoState& state) override;
    bool onModeLongPress(MotoState& state) override;
    bool onActionClick(MotoState& state) override;
    bool onActionLongPress(MotoState& state) override;

private:
    // Widoki ekranu
    enum View : uint8_t { VIEW_MAIN = 0, VIEW_CALIB, VIEW_RESULT };

    // Pozycje przycisków (te same miejsca w obu widokach, inne podpisy):
    //   widok główny:  -  +  DO PELNA  DOLEJ   ANULUJ
    //   kalibracja:    -  +  ZAPISZ    POMIN   WSTECZ
    enum Item : uint8_t { ITEM_MINUS = 0, ITEM_PLUS, ITEM_FULL, ITEM_ADD, ITEM_CANCEL, ITEM_COUNT };

    FuelCalculator* fuelCalculator;
    ScreenManager*  screenManager;
    RefuelLog*      refuelLog;

    View    view          = VIEW_MAIN;
    float   amountL       = 5.0f;           // Wybrana ilość do dolania [L]
    float   pumpedL       = 0.0f;           // Litry z dystrybutora wpisywane przy kalibracji [L]
    uint8_t selected      = ITEM_PLUS;      // Zaznaczona opcja (sterowanie przyciskami)
    bool    controlsDirty = true;           // Przerysuj przyciski i wybraną ilość
    unsigned long lastFuelDrawMs = 0;       // Ostatnie odświeżenie karty stanu paliwa
    float   lastDrawnFuelL = -1.0f;         // Ostatnio narysowana ilość paliwa (odświeżamy tylko po zmianie)
    MotoState*    statePtr       = nullptr; // Stan aplikacji (potrzebny do automatycznego zamknięcia w update())
    unsigned long lastActivityMs = 0;       // Czas ostatniego naciśnięcia przycisku na tym ekranie
    int           lastShownSec   = -1;      // Ostatnio narysowana liczba sekund do automatycznego powrotu

    // Wynik zamknięcia baku (do widoku wyniku)
    RefuelResult lastResult;

    float freeSpaceL(const MotoState& state) const; // Ile litrów zmieści się jeszcze w baku
    float maxAmountL(const MotoState& state) const; // Maksymalna ilość do wyboru (wolne miejsce w baku)
    bool  isTankFull(const MotoState& state) const; // Bak pełny -> -, + i DOLEJ są nieaktywne
    bool  isItemEnabled(const MotoState& state, uint8_t item) const;
    void  changeAmount(const MotoState& state, int steps);
    void  changePumped(const MotoState& state, int steps);
    void  activate(MotoState& state, uint8_t item);      // Akcje widoku głównego
    void  activateCalib(MotoState& state, uint8_t item); // Akcje widoku kalibracji
    void  startFullRefuel(MotoState& state);             // DO PELNA: kalibracja (jeśli możliwa) lub od razu pełny bak
    void  finishCalibration(MotoState& state, bool save);
    void  closeTank(MotoState& state, float pumpedL, bool measurable); // Historia + zerowanie licznika baku
    void  switchView(MotoState& state, View v, uint8_t sel);
    void  close(MotoState& state);
    void  touchActivity() { lastActivityMs = millis(); } // Resetuje licznik bezczynności

    void drawHeader(TFT_eSPI& tft, const char* title);
    void drawCountdown(TFT_eSPI& tft, unsigned long timeoutMs);
    void drawFuelCard(TFT_eSPI& tft, const MotoState& state);
    void drawControls(TFT_eSPI& tft, const MotoState& state);
    void drawCalibInfo(TFT_eSPI& tft, const MotoState& state);
    void drawCalibControls(TFT_eSPI& tft, const MotoState& state);
    void drawResult(TFT_eSPI& tft, const MotoState& state);
    void drawButton(TFT_eSPI& tft, int x, int y, int w, int h, const String& label,
                    uint16_t accent, bool isSelected, uint8_t font);
};
