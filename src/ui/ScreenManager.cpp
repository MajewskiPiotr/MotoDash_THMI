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
    // tft.pushImage(posX, posY, LOGO_WIDTH, LOGO_HEIGHT, bootLogo);

    // delay(1800); // Wyświetla logo przez 1.8 sekundy przed przejściem do Ekranu
}
// Cofnięcie do poprzedniego ekranu (wywoływane przy przesunięciu palcem w prawo)
void ScreenManager::prevScreen(MotoState& state) {
    if (screenCount == 0) return;
    if (overlayScreen) { closeOverlay(state); return; } // Gest na nakładce tylko ją zamyka
    // Dodajemy screenCount przed modulo, aby uniknąć ujemnych indeksów w C++
    activeIndex = (activeIndex + screenCount - 1) % screenCount;
    state.forceRedraw = true;
}
void ScreenManager::playBootAnimation() {
    const int SCREEN_W = 320;
    const int SCREEN_H = 240;
    const int LOGO_X   = (SCREEN_W - LOGO_WIDTH) / 2;
    const int LOGO_Y   = 32;
    const uint16_t BENELLI_GREEN = 0x0545;
    const uint16_t BENELLI_RED   = 0xD800;
    const uint16_t RING_BG       = 0x2104;

    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(true);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    // Parametry paska postępu
    const int barW = 240;
    const int barH = 6;
    const int barX = (SCREEN_W - barW) / 2;
    const int barY = 218;

    // Rysujemy pustą ramkę paska na start
    tft.drawRect(barX - 2, barY - 2, barW + 4, barH + 4, RING_BG);

    const char* brandName = "BENELLI";
    int lastDrawnRows = 0;
    int lastDrawnLetters = 0;
    bool stripeDrawn = false;

    // Główna pętla postępu od 0 do 100% (krok co 2%)
    for (int pct = 0; pct <= 100; pct += 2) {
        // 1. Pasek postępu
        int currentBarW = (barW * pct) / 100;
        uint16_t barColor = (pct < 75) ? BENELLI_GREEN : BENELLI_RED;
        if (currentBarW > 0) {
            tft.fillRect(barX, barY, currentBarW, barH, barColor);
        }

        // 2. Odsłanianie logo od dołu do góry proporcjonalnie do % paska
        int targetRows = (LOGO_HEIGHT * pct) / 100;
        while (lastDrawnRows < targetRows) {
            int rowIdx = LOGO_HEIGHT - 1 - lastDrawnRows; // Liczymy od dolnej krawędzi w górę
            tft.pushImage(LOGO_X, LOGO_Y + rowIdx, LOGO_WIDTH, 1, &bootLogo[rowIdx * LOGO_WIDTH]);
            lastDrawnRows++;
        }

        // 3. Włoski pasek pojawia się w połowie ładowania (50%)
        if (pct >= 50 && !stripeDrawn) {
            int stripeW = 150;
            int partW = stripeW / 3;
            int startX = (SCREEN_W - stripeW) / 2;
            int stripeY = LOGO_Y + LOGO_HEIGHT + 8;
            
            tft.fillRect(startX, stripeY, partW, 4, BENELLI_GREEN);
            tft.fillRect(startX + partW, stripeY, partW, 4, TFT_WHITE);
            tft.fillRect(startX + 2 * partW, stripeY, partW, 4, BENELLI_RED);
            stripeDrawn = true;
        }

        // 4. Napis BENELLI dopisuje kolejne litery między 30% a 100%
        if (pct >= 30) {
            int targetLetters = ((pct - 30) * 7) / 70;
            if (targetLetters > 7) targetLetters = 7;
            
            if (targetLetters > lastDrawnLetters) {
                String sub = String(brandName).substring(0, targetLetters);
                tft.drawString(sub, SCREEN_W / 2, 190, 4);
                lastDrawnLetters = targetLetters;
            }
        }

        delay(22); // Łączny czas ładowania: ok. 1.1 sekundy (zmniejsz/zwiększ według uznania)
    }

    // ZAMROŻENIE EKRANU NA 2 SEKUNDY PO NARYSOWANIU
    delay(2000);
    // 5. Kurtyna końcowa zamykająca ekran
    for (int y = 0; y <= SCREEN_H / 2; y += 3) {
        tft.fillRect(0, 0, SCREEN_W, y, TFT_BLACK);
        tft.fillRect(0, SCREEN_H - y, SCREEN_W, y, TFT_BLACK);
        delay(5);
    }
    tft.fillScreen(TFT_BLACK);

    // Przywrócenie ustawień dla ekranów jazdy
    tft.setTextDatum(TL_DATUM);
    tft.setSwapBytes(false);
}



void ScreenManager::registerScreen(Screen* screen) {
    // Zapisuje wskaźnik ekranu w tablicy, jeśli nie przekroczono maksymalnej liczby ekranów
    if (screenCount < MAX_SCREENS && screen != nullptr) {
        screens[screenCount++] = screen;
    }
}

void ScreenManager::nextScreen(MotoState& state) {
    if (screenCount == 0) return;
    if (overlayScreen) { closeOverlay(state); return; } // Gest/przycisk na nakładce tylko ją zamyka
    activeIndex = (activeIndex + 1) % screenCount; // Przechodzi cyklicznie do następnego ekranu
    state.forceRedraw = true;                      // Wymusza natychmiastowe narysowanie tła nowego ekranu
}

void ScreenManager::goToFirstScreen(MotoState& state) {
    overlayScreen = nullptr;  // Zamyka ewentualną nakładkę
    activeIndex = 0;          // Ustawia indeks pierwszego ekranu (Tryb Jazdy)
    state.forceRedraw = true; // Wymusza natychmiastowe przerysowanie
}

Screen* ScreenManager::getActiveScreen() {
    if (overlayScreen) return overlayScreen; // Nakładka ma pierwszeństwo przed karuzelą
    if (screenCount == 0) return nullptr;
    return screens[activeIndex];
}

void ScreenManager::showOverlay(Screen* overlay, MotoState& state) {
    if (!overlay) return;
    overlayScreen = overlay;
    state.forceRedraw = true; // Narysuje nakładkę od zera w najbliższym render()
}

void ScreenManager::closeOverlay(MotoState& state) {
    if (!overlayScreen) return;
    overlayScreen = nullptr;
    state.forceRedraw = true; // Przerysowuje ekran, który był pod nakładką
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
        if (!overlayScreen) drawPageIndicator(); // Kropki stron tylko dla ekranów z karuzeli
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
