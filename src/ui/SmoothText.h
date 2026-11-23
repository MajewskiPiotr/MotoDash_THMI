#pragma once
#include <TFT_eSPI.h>

// Tekst gładką czcionką (z wygładzaniem krawędzi) rysowany przez bufor w pamięci (sprite):
// całe pole w x h powstaje w pamięci i trafia na ekran jednym ruchem - bez migania.
namespace SmoothText {
    enum Align : uint8_t { LEFT = 0, CENTER = 1, RIGHT = 2 };

    // Pole (x, y, w, h) wypełnione kolorem bg, tekst wyrównany poziomo i wyśrodkowany w pionie
    void draw(TFT_eSPI& tft, const uint8_t* font, const String& text,
              int x, int y, int w, int h, Align align, uint16_t fg, uint16_t bg);

    // Zamiennik tft.drawString / drawCentreString / drawRightString z gładką czcionką Inter.
    // size jak we wbudowanych czcionkach: 1 = podpisy (11 px), 2 = tekst (15 px), 4 = duże napisy (24 px).
    // Kolory i szerokość tła bierze z bieżących ustawień tft (setTextColor z tłem! / setTextPadding).
    void str(TFT_eSPI& tft, const String& text, int x, int y, uint8_t size, Align align = LEFT);
    int  height(TFT_eSPI& tft, uint8_t size); // Wysokość wiersza czcionki o danym rozmiarze [px]

    // Pole z pamięcią ostatniej wartości - rysuje się ponownie tylko, gdy zmieni się tekst lub kolory
    struct Field {
        String   text;
        uint16_t fg = 0, bg = 0;
        void reset() { text = String((char)1); } // Wymusza narysowanie przy następnym show()
        void show(TFT_eSPI& tft, const uint8_t* font, const String& t,
                  int x, int y, int w, int h, Align align, uint16_t fgc, uint16_t bgc);
    };
}
