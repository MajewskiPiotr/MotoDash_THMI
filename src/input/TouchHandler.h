#pragma once
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "MotoState.h"
#include "ScreenManager.h"

// Klasa obsługująca rezystancyjny panel dotykowy (XPT2046) na płytce LilyGO T-HMI.
// Wykrywa gesty przesunięcia (Swipe w lewo / w prawo) oraz krótkie dotknięcia.
class TouchHandler {
public:
    void init();                                                 // Uruchamia magistralę SPI dla dotyku
    void update(MotoState& state, ScreenManager& screenManager); // Odczytuje dotyk i rozpoznaje gesty

private:
    // Fizyczne piny panelu dotykowego na płytce LilyGO T-HMI
    static constexpr uint8_t TOUCH_CS   = 2;
    static constexpr uint8_t TOUCH_IRQ  = 9;
    static constexpr uint8_t TOUCH_CLK  = 1;
    static constexpr uint8_t TOUCH_MISO = 4;
    static constexpr uint8_t TOUCH_MOSI = 3;

    // Minimalna długość ruchu palcem (w surowych jednostkach dotyku), aby uznać gest za przesunięcie
    static constexpr int SWIPE_THRESHOLD = 450;

    SPIClass touchSpi = SPIClass(HSPI); // Wydzielona magistrala SPI dla kontrolera dotyku
    XPT2046_Touchscreen ts = XPT2046_Touchscreen(TOUCH_CS, TOUCH_IRQ);

    bool isTouching = false; // Czy palec aktualnie dotyka ekranu
    int  startX     = 0;     // Pozycja X w chwili dotknięcia ekranu
    int  lastX      = 0;     // Ostatnia odczytana pozycja X przed oderwaniem palca
    unsigned long lastPollMs = 0;
};
