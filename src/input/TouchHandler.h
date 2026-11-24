#pragma once
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "MotoState.h"
#include "ScreenManager.h"

// Rezystancyjny panel dotykowy (XPT2046) płytki LilyGO T-HMI - jedyne sterowanie MotoDash.
// Działa także w rękawicach (reaguje na nacisk, nie na skórę).
//  - przesunięcie w lewo / w prawo  -> następny / poprzedni ekran,
//  - stuknięcie                      -> przycisk / pole w miejscu dotyku (Screen::onTap),
//  - przytrzymanie ~0,8 s            -> Screen::onLongTouch (zerowanie, kasowanie itp.),
//  - stuknięcie w górny pasek (y < 20), jeśli ekran go nie używa -> jasność dzień / noc.
// Kalibracja 3-punktowa zapisywana w pamięci Flash. Uruchamia się sama przy pierwszym starcie,
// gdy trzymasz palec na ekranie podczas uruchamiania, albo z ekranu Ustawień.
class TouchHandler {
public:
    void init();
    void update(MotoState& state, ScreenManager& screenManager);

    bool isCalibrated() const { return calibrated; }
    bool isTouchedNow();                        // Czy palec jest teraz na ekranie (do wymuszenia kalibracji)
    void calibrate(TFT_eSPI& tft);              // Blokujące: 3 krzyżyki do dotknięcia, zapis do Flash

    // Przelicza surowy odczyt na piksele ekranu 320x240 (public do testów)
    void rawToScreen(int rx, int ry, int& sx, int& sy) const;

private:
    static constexpr uint8_t TOUCH_CS   = 2;
    static constexpr uint8_t TOUCH_IRQ  = 9;
    static constexpr uint8_t TOUCH_CLK  = 1;
    static constexpr uint8_t TOUCH_MISO = 4;
    static constexpr uint8_t TOUCH_MOSI = 3;

    static constexpr int           MIN_Z        = 120;  // Minimalny nacisk (szumy poniżej)
    static constexpr int           SWIPE_PX     = 60;   // Ruch w poziomie uznawany za przesunięcie
    static constexpr int           TAP_MOVE_PX  = 22;   // Większy ruch = to nie jest stuknięcie
    static constexpr unsigned long LONG_MS      = 800;  // Przytrzymanie
    static constexpr uint8_t       RELEASE_POLLS = 2;   // Tyle odczytów "brak dotyku" = palec oderwany

    XPT2046_Touchscreen ts = XPT2046_Touchscreen(TOUCH_CS, TOUCH_IRQ);

    // Kalibracja: ekran X = ax * raw[axisX] + bx, ekran Y = ay * raw[1 - axisX] + by
    bool  calibrated = false;
    uint8_t axisX = 0;
    float ax = 320.0f / 3600.0f, bx = -320.0f * 250 / 3600.0f;
    float ay = 240.0f / 3600.0f, by = -240.0f * 250 / 3600.0f;

    // Gest w toku
    bool down = false, longFired = false;
    uint8_t releaseCnt = 0;
    int startX = 0, startY = 0, lastX = 0, lastY = 0;
    unsigned long downMs = 0, lastPollMs = 0;

    bool readPoint(int& sx, int& sy);           // Odczyt w pikselach ekranu (false = brak dotyku)
    bool waitPress(int& rawX, int& rawY);        // Kalibracja: średnia z dotknięcia, czeka na oderwanie
    void loadCalibration();
    void saveCalibration();
    void toggleBrightness(MotoState& state);
};
