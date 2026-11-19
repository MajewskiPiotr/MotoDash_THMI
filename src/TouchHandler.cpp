#include "TouchHandler.h"

void TouchHandler::init() {
    // Inicjalizujemy globalną magistralę SPI dokładnie na pinach dotyku LilyGO T-HMI
    SPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);

    // Uruchamiamy sterownik dotyku (korzysta z zainicjalizowanej wyżej globalnej magistrali SPI)
    ts.begin();
    ts.setRotation(1); // Dopasowujemy orientację dotyku do poziomego ułożenia ekranu (320x240)

    Serial.println("[TouchHandler] Panel dotykowy XPT2046 uruchomiony");
}

void TouchHandler::update(MotoState& state, ScreenManager& screenManager) {
    // Odczytujemy panel dotykowy co 30 ms (~33 razy na sekundę - w pełni płynne wykrywanie gestów)
    if (millis() - lastPollMs < 30) return;
    lastPollMs = millis();

    // 1. Gdy palec lub rysik dotyka ekranu:
    if (ts.touched()) {
        TS_Point p = ts.getPoint(); // Pobieramy surowy punkt dotyku (x, y, siła nacisku z)

        // Ignorujemy fałszywe odczyty o bardzo małej sile nacisku (szumy)
        if (p.z < 100) return;

        if (!isTouching) {
            // Pierwszy moment dotknięcia - zapamiętujemy punkt startowy gestu
            isTouching = true;
            startX = p.x;
        }
        // Na bieżąco aktualizujemy ostatnią znaną pozycję palca
        lastX = p.x;
    } 
    // 2. Gdy palec został właśnie oderwany od ekranu (koniec gestu):
    else if (isTouching) {
        isTouching = false;
        int deltaX = lastX - startX; // Obliczamy przesunięcie w poziomie względem punktu startu

        if (deltaX < -SWIPE_THRESHOLD) {
            // Przesunięcie w LEWO -> przejdź do następnego ekranu w karuzeli
            Serial.println("[Touch] Swipe w LEWO -> Nastepny ekran");
            screenManager.nextScreen(state);
        } 
        else if (deltaX > SWIPE_THRESHOLD) {
            // Przesunięcie w PRAWO -> wróć do poprzedniego ekranu w karuzeli
            Serial.println("[Touch] Swipe w PRAWO -> Poprzedni ekran");
            screenManager.prevScreen(state);
        } 
        else {
            // Krótkie dotknięcie w miejscu (Tap) -> działa jak kliknięcie MODE (np. przesuwa kursor w menu)
            Serial.println("[Touch] Klikniecie (Tap)");
            Screen* active = screenManager.getActiveScreen();
            if (active && active->onModeClick(state)) {
                return;
            }
        }
    }
}
