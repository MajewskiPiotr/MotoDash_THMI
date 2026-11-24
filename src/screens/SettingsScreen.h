#pragma once
#include "Screen.h"
#include "ScreenManager.h"

class TouchHandler;

// Ekran 4: Menu konfiguracji z trwałym zapisem ustawień do nieulotnej pamięci Flash (Preferences).
class SettingsScreen : public Screen {
public:
    explicit SettingsScreen(ScreenManager* mgr) : screenManager(mgr) {}

    void loadFromNvs(MotoState& state);       // Wczytuje zapisane ustawienia z Flash przy starcie urządzenia
    void saveToNvs(const MotoState& state);   // Zapisuje zmodyfikowane ustawienia do pamięci Flash

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;

    // Nadpisujemy zachowanie przycisków, aby wewnątrz menu służyły do nawigacji i edycji wartości
    bool onTap(MotoState& state, int x, int y) override;       // Wiersz = wybór, [-]/[+] = zmiana, GOTOWE = wyjście
    bool onLongTouch(MotoState& state, int x, int y) override; // Przytrzymanie [-]/[+] = 5 kroków

    void setTouchHandler(TouchHandler* t) { touch = t; } // Do kalibracji dotyku z menu

private:
    ScreenManager* screenManager;
    TouchHandler*  touch = nullptr;
    uint8_t selectedRow = 0;                 // Aktualnie podświetlony wiersz menu (0..5)
    bool isDirty = true;                     // Flaga "brudnego ekranu" - rysuje listę tylko po kliknięciu guzika
    static constexpr uint8_t TOTAL_ROWS = 6; // Łączna liczba pozycji w menu konfiguracji
    static constexpr uint8_t ROW_TOUCH_CAL = 5; // Ostatni wiersz: kalibracja dotyku (bez wartości)

    void changeValue(MotoState& state, int steps);
};
