#pragma once
#include <TFT_eSPI.h>
#include "MotoState.h"

// Abstrakcyjna klasa bazowa (odpowiednik interfejsu w Javie) dla kaĹĽdego ekranu w aplikacji.
class Screen {
public:
    virtual ~Screen() = default; // Wirtualny destruktor wymagany przy dziedziczeniu w C++

    // WywoĹ‚ywana RAZ w momencie przeĹ‚Ä…czenia na ten ekran (rysuje statyczne ramki i napisy)
    virtual void onEnter(TFT_eSPI& tft, const MotoState& state) = 0;

    // WywoĹ‚ywana cyklicznie w pÄ™tli gĹ‚Ăłwnej (odĹ›wieĹĽa wyĹ‚Ä…cznie zmieniajÄ…ce siÄ™ wartoĹ›ci)
    virtual void update(TFT_eSPI& tft, const MotoState& state) = 0;

    // Opcjonalne metody obsĹ‚ugujÄ…ce klikniÄ™cia/przytrzymania na konkretnym ekranie.
    // ZwracajÄ… 'true', jeĹ›li ekran sam obsĹ‚uĹĽyĹ‚ przycisk (blokujÄ…c domyĹ›lnÄ… akcjÄ™ globalnÄ…).
    virtual bool onModeClick(MotoState& state)       { return false; }
    virtual bool onModeLongPress(MotoState& state)   { return false; }
    virtual bool onActionClick(MotoState& state)     { return false; }
    virtual bool onActionLongPress(MotoState& state) { return false; }

    // Podwójne kliknięcie ACTION. Jeśli ekran go nie obsłuży (false), ButtonHandler
    // traktuje je jak dwa zwykłe kliknięcia ACTION (np. dwa razy "+" w menu).
    virtual bool onActionDoubleClick(MotoState& state) { return false; }
};
