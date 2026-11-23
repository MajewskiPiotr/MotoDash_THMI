#include "Icons.h"

namespace Icons {

void thermometer(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg) {
    t.fillRoundRect(x + 7, y, 6, 14, 3, c);      // Rurka
    t.fillRoundRect(x + 9, y + 2, 2, 9, 1, bg);  // Wnętrze rurki
    t.fillRect(x + 9, y + 7, 2, 6, c);           // Słupek rtęci
    t.fillCircle(x + 10, y + 15, 4, c);          // Zbiorniczek
    for (int k = 3; k <= 9; k += 3) t.drawFastHLine(x + 14, y + k, 3, c); // Podziałka
}

void droplet(TFT_eSPI& t, int x, int y, uint16_t c) {
    t.fillCircle(x + 10, y + 13, 6, c);
    t.fillTriangle(x + 10, y + 1, x + 4, y + 11, x + 16, y + 11, c);
}

void battery(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg) {
    t.fillRect(x + 4, y + 3, 3, 2, c);           // Biegun +
    t.fillRect(x + 13, y + 3, 3, 2, c);          // Biegun -
    t.fillRoundRect(x + 1, y + 5, 18, 12, 2, c);
    t.fillRoundRect(x + 3, y + 7, 14, 8, 1, bg);
    t.drawFastHLine(x + 5, y + 11, 4, c);        // "+"
    t.drawFastVLine(x + 7, y + 9, 5, c);
    t.drawFastHLine(x + 11, y + 11, 4, c);       // "-"
}

void cloud(TFT_eSPI& t, int x, int y, uint16_t c) {
    t.fillCircle(x + 7, y + 11, 4, c);
    t.fillCircle(x + 12, y + 8, 5, c);
    t.fillCircle(x + 16, y + 12, 3, c);
    t.fillRoundRect(x + 3, y + 11, 16, 5, 2, c);
}

void fuelPump(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg) {
    t.fillRoundRect(x + 2, y + 2, 10, 15, 2, c); // Korpus
    t.fillRect(x + 4, y + 4, 6, 4, bg);          // Okienko
    t.fillRect(x + 1, y + 17, 12, 2, c);         // Podstawa
    t.drawLine(x + 12, y + 6, x + 15, y + 8, c); // Wąż
    t.drawFastVLine(x + 15, y + 8, 7, c);
    t.drawFastVLine(x + 16, y + 8, 7, c);
    t.drawLine(x + 15, y + 15, x + 13, y + 15, c);
    t.fillRect(x + 14, y + 3, 3, 3, c);          // Pistolet
}

void road(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg) {
    t.fillTriangle(x + 7, y + 1, x + 13, y + 1, x + 19, y + 19, c);
    t.fillTriangle(x + 7, y + 1, x + 19, y + 19, x + 1, y + 19, c);
    t.fillRect(x + 9, y + 4, 2, 3, bg);          // Przerywana linia środkowa
    t.fillRect(x + 9, y + 9, 2, 3, bg);
    t.fillRect(x + 9, y + 14, 2, 4, bg);
}

void tire(TFT_eSPI& t, int x, int y, uint16_t c, uint16_t bg) {
    t.fillCircle(x + 10, y + 10, 9, c);
    t.fillCircle(x + 10, y + 10, 5, bg);
    t.fillCircle(x + 10, y + 10, 2, c);
    static const int8_t notch[8][2] = {{9,0},{9,18},{0,9},{18,9},{3,3},{15,3},{3,15},{15,15}}; // Bieżnik
    for (auto& n : notch) t.fillRect(x + n[0], y + n[1], 2, 2, bg);
}

void degree(TFT_eSPI& t, int x, int y, uint16_t c) {
    t.drawCircle(x, y, 2, c);
}

} // namespace Icons
