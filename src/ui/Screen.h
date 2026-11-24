#pragma once
#include <TFT_eSPI.h>
#include "MotoState.h"

// Prostokątny obszar dotyku na ekranie (np. narysowany przycisk)
struct TouchRect {
    int x, y, w, h;
    bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

// Abstrakcyjna klasa bazowa dla każdego ekranu w aplikacji.
// Sterowanie dotykiem (TouchHandler):
//  - przesunięcie palcem w lewo / w prawo  -> następny / poprzedni ekran (robi to ScreenManager),
//  - stuknięcie                             -> onTap(x, y),
//  - przytrzymanie ~0,8 s                   -> onLongTouch(x, y) (akcje, których nie chcemy wywołać przypadkiem).
// Metody zwracają true, jeśli ekran obsłużył dotyk w tym miejscu.
class Screen {
public:
    virtual ~Screen() = default;

    // Wywoływana RAZ w momencie przełączenia na ten ekran (rysuje statyczne ramki i napisy)
    virtual void onEnter(TFT_eSPI& tft, const MotoState& state) = 0;

    // Wywoływana cyklicznie w pętli głównej (odświeża wyłącznie zmieniające się wartości)
    virtual void update(TFT_eSPI& tft, const MotoState& state) = 0;

    virtual bool onTap(MotoState& state, int x, int y)       { return false; }
    virtual bool onLongTouch(MotoState& state, int x, int y) { return false; }
};
