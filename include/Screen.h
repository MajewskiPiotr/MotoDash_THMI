#pragma once
#include <TFT_eSPI.h>
#include "MotoState.h"

// Abstrakcyjna klasa bazowa (odpowiednik interfejsu w Javie) dla każdego ekranu w aplikacji.
class Screen {
public:
    virtual ~Screen() = default; // Wirtualny destruktor wymagany przy dziedziczeniu w C++

    // Wywoływana RAZ w momencie przełączenia na ten ekran (rysuje statyczne ramki i napisy)
    virtual void onEnter(TFT_eSPI& tft, const MotoState& state) = 0;

    // Wywoływana cyklicznie w pętli głównej (odświeża wyłącznie zmieniające się wartości)
    virtual void update(TFT_eSPI& tft, const MotoState& state) = 0;

    // Opcjonalne metody obsługujące kliknięcia/przytrzymania na konkretnym ekranie.
    // Zwracają 'true', jeśli ekran sam obsłużył przycisk (blokując domyślną akcję globalną).
    virtual bool onModeClick(MotoState& state)       { return false; }
    virtual bool onModeLongPress(MotoState& state)   { return false; }
    virtual bool onActionClick(MotoState& state)     { return false; }
    virtual bool onActionLongPress(MotoState& state) { return false; }

    // Podwójne kliknięcie ACTION. Jeśli ekran go nie obsłuży (false), ButtonHandler
    // traktuje je jak dwa zwykłe kliknięcia ACTION (np. dwa razy "+" w menu).
    virtual bool onActionDoubleClick(MotoState& state) { return false; }
};
