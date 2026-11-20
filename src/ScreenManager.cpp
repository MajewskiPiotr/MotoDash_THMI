#include "ScreenManager.h"
#include "Config.h"
#include "BootLogo.h" // Dołączamy plik z naszą grafiką startową

void ScreenManager::init() {
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH); // 1. Włącza zasilanie 3.3V matrycy (PWR_EN)

    delay(100);

    tft.init();             // 2. Inicjalizuje sterownik ST7789
    tft.invertDisplay(false); // Naprawia wyblakłe/odwrócone kolory matrycy (jeśli bez zmian, wpisz 'true')
        tft.setRotation(1);     // Orientacja pozioma 320x240
    tft.setSwapBytes(true);   // 5. Wymagane dla pushImage(), aby kolory R i B nie były zamienione miejscami

    pinMode(38, OUTPUT);
    digitalWrite(38, HIGH); // 3. Włącza podświetlenie LED ekranu (GPIO 38)
      // --- WYŚWIETLENIE EKRANU POWITALNEGO ---
    tft.fillScreen(TFT_BLACK); // Czyści ekran na czarno (idealne tło pod logo)

    // Wyśrodkowuje grafikę na ekranie (dla 320x240 będzie to punkt X=0, Y=0)
    int posX = (320 - LOGO_WIDTH) / 2;
    int posY = (240 - LOGO_HEIGHT) / 2;

    // Rysuje cały obrazek z pamięci Flash w ułamku sekundy
    tft.pushImage(posX, posY, LOGO_WIDTH, LOGO_HEIGHT, bootLogo);

    delay(1800); // Wyświetla logo przez 1.8 sekundy przed przejściem do Ekranu
}
// Cofnięcie do poprzedniego ekranu (wywoływane przy przesunięciu palcem w prawo)
void ScreenManager::prevScreen(MotoState& state) {
    if (screenCount == 0) return;
    // Dodajemy screenCount przed modulo, aby uniknąć ujemnych indeksów w C++
    activeIndex = (activeIndex + screenCount - 1) % screenCount;
    state.forceRedraw = true;
}
void ScreenManager::registerScreen(Screen* screen) {
    // Zapisuje wskaźnik ekranu w tablicy, jeśli nie przekroczono maksymalnej liczby ekranów
    if (screenCount < MAX_SCREENS && screen != nullptr) {
        screens[screenCount++] = screen;
    }
}

void ScreenManager::nextScreen(MotoState& state) {
    if (screenCount == 0) return;
    activeIndex = (activeIndex + 1) % screenCount; // Przechodzi cyklicznie do następnego ekranu
    state.forceRedraw = true;                      // Wymusza natychmiastowe narysowanie tła nowego ekranu
}

void ScreenManager::goToFirstScreen(MotoState& state) {
    activeIndex = 0;          // Ustawia indeks pierwszego ekranu (Tryb Jazdy)
    state.forceRedraw = true; // Wymusza natychmiastowe przerysowanie
}

Screen* ScreenManager::getActiveScreen() {
    if (screenCount == 0) return nullptr;
    return screens[activeIndex];
}

void ScreenManager::render(MotoState& state) {
    Screen* current = getActiveScreen();
    if (!current) return;

    // 1. Natychmiastowe pełne przerysowanie po zmianie ekranu lub wymuszeniu przez przycisk
    if (state.forceRedraw || activeIndex != lastRenderedIndex) {
        state.forceRedraw = false;
        lastRenderedIndex = activeIndex;
        lastRenderMs = millis(); // Resetuje licznik czasu odświeżania

        current->onEnter(tft, state); // Rysuje statyczne tło, ramki i etykiety nowego ekranu
        drawPageIndicator();          // Rysuje kropki stron w prawym górnym rogu
        current->update(tft, state);  // Od razu wpisuje pierwsze wartości liczbowe
        return;
    }

    // 2. Cykliczne odświeżanie samych wartości nie częściej niż co 150 ms (eliminuje migotanie)
    unsigned long now = millis();
    if (now - lastRenderMs >= REFRESH_INTERVAL_MS) {
        lastRenderMs = now;
        current->update(tft, state); // Aktualizuje wyłącznie dynamiczne dane na wybranym ekranie
    }
}

void ScreenManager::drawPageIndicator() {
    // Oblicza pozycję startową kropek w prawym górnym rogu ekranu
    int startX = 308 - (screenCount * 12);
    for (uint8_t i = 0; i < screenCount; i++) {
        // Aktywna strona rysowana jest na zielono, pozostałe na ciemnoszaro
        uint16_t color = (i == activeIndex) ? 0x07E0 : 0x4A69;
        tft.fillCircle(startX + (i * 12), 10, 3, color);
    }
}
