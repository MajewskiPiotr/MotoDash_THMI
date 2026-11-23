#pragma once
#include <Arduino.h>

// Gładkie czcionki (Inter, licencja OFL) wygenerowane narzędziem tools/make_vlw.py.
// Liczbowe zawierają tylko: cyfry . - ° spacja. Tekstowe (Label/Text/Title): litery z polskimi znakami, cyfry i symbole.
extern const uint8_t FontNumXL[]; // 57 px - duże liczby (temperatura, spalanie)
extern const uint8_t FontNumL[];  // 29 px - średnie liczby (napięcie, paliwo, zasięg)
extern const uint8_t FontNumM[];  // 19 px - małe liczby (opony, średnie spalanie)
extern const uint8_t FontLabel[]; // 11 px - podpisy
extern const uint8_t FontText[];  // 15 px - zwykły tekst (menu, tabele, opisy)
extern const uint8_t FontTitle[]; // 24 px - duże napisy i wartości z jednostkami
