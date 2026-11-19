#pragma once
#include "Screen.h"
#include "ScreenManager.h"

// Ekran 4: Menu konfiguracji z trwałym zapisem ustawień do nieulotnej pamięci Flash (Preferences).
class SettingsScreen : public Screen {
public:
    explicit SettingsScreen(ScreenManager* mgr) : screenManager(mgr) {}

    void loadFromNvs(MotoState& state);       // Wczytuje zapisane ustawienia z Flash przy starcie urządzenia
    void saveToNvs(const MotoState& state);   // Zapisuje zmodyfikowane ustawienia do pamięci Flash

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;

    // Nadpisujemy zachowanie przycisków, aby wewnątrz menu służyły do nawigacji i edycji wartości
    bool onModeClick(MotoState& state) override;       // Przesuwa kursor na kolejną opcję w dół
    bool onModeLongPress(MotoState& state) override;   // Zapisuje zmiany i wychodzi do ekranu jazdy
    bool onActionClick(MotoState& state) override;     // Zwiększa wybraną wartość (+) lub zatwierdza wyjście
    bool onActionLongPress(MotoState& state) override; // Zmniejsza wybraną wartość (-)

private:
    ScreenManager* screenManager;
    uint8_t selectedRow = 0;                 // Aktualnie podświetlony wiersz menu (0..5)
    bool isDirty = true;                     // Flaga "brudnego ekranu" - rysuje listę tylko po kliknięciu guzika
    static constexpr uint8_t TOTAL_ROWS = 6; // Łączna liczba pozycji w menu konfiguracji
};
