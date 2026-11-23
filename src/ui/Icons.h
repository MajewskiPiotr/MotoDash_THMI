#pragma once
#include <TFT_eSPI.h>

// Proste ikony 20x20 px rysowane prymitywami TFT (bez bitmap - nie zajmują miejsca w pamięci).
// (x, y) = lewy górny róg pola 20x20; c = kolor ikony; bg = kolor tła pod ikoną (do "wycięć").
namespace Icons {
    void thermometer(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg);
    void droplet(TFT_eSPI& t, int x, int y, uint16_t c);
    void battery(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg);
    void cloud(TFT_eSPI& t, int x, int y, uint16_t c);
    void fuelPump(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg);
    void road(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg);
    void tire(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg);
    void degree(TFT_eSPI& t, int x, int y, uint16_t c); // Mały znak stopnia (środek w x, y)
    void speedometer(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg);
    void warning(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg);   // Trójkąt z wykrzyknikiem
    void mountain(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg);
    void check(TFT_eSPI& t, int x, int y, uint16_t c);                  // "Ptaszek"
    void wifi(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg);      // Pole 32x22 (gładkie łuki)
}
