#pragma once
#include <TFT_eSPI.h>
#include "Screen.h"
#include "MotoState.h"

// Kontroler zarządzający wyświetlaczem, tablicą zarejestrowanych ekranów oraz nawigacją między nimi.
class ScreenManager {
public:
    void init();                           // Inicjalizuje sprzętowo ekran TFT i włącza jego zasilanie
    void playBootAnimation();
    void registerScreen(Screen* screen);   // Dodaje nowy ekran do listy dostępnych widoków (karuzeli)
    void nextScreen(MotoState& state);     // Przełącza na kolejny ekran w pętli (0 -> 1 -> 2 -> 0)
    void goToFirstScreen(MotoState& state);// Wraca bezpośrednio do pierwszego ekranu (Tryb Jazdy)
    void render(MotoState& state);         // Zarządza częstotliwością odświeżania i rysowaniem aktywnego ekranu
    void prevScreen(MotoState& state); // Przełącza na poprzedni ekran w karuzeli (np. 2 -> 1 -> 0)
    Screen* getActiveScreen();             // Zwraca wskaźnik do aktualnie wyświetlanego ekranu

private:
    static constexpr uint8_t MAX_SCREENS = 6;                 // Maksymalna liczba ekranów w aplikacji
    static constexpr unsigned long REFRESH_INTERVAL_MS = 150; // Minimalny odstęp między odświeżeniami ekranu (~6.5 FPS)

    TFT_eSPI tft = TFT_eSPI();                // Jedyna instancja sterownika wyświetlacza w systemie
    Screen* screens[MAX_SCREENS] = { nullptr }; // Tablica wskaźników na obiekty poszczególnych ekranów
    uint8_t screenCount = 0;                  // Liczba zarejestrowanych ekranów
    uint8_t activeIndex = 0;                  // Indeks aktualnie wyświetlanego ekranu
    uint8_t lastRenderedIndex = 255;          // Zapamiętany poprzedni indeks (służy do wykrycia zmiany widoku)
    unsigned long lastRenderMs = 0;           // Znacznik czasu ostatniego odświeżenia ekranu [ms]

    void drawPageIndicator();                 // Rysuje kropki stron (np. ● ○ ○) w prawym górnym rogu
};
