#pragma once
#include "Screen.h"

// Ekran prezentujący tabelę 5 ostatnich zakończonych tras (czas, dystans, średnie spalanie, średnia prędkość)
// wraz z odczytem/zapisem do pliku CSV na karcie microSD.
class AnaliticScreen : public Screen {
public:
    void initSdAndLoad(MotoState& state); // Inicjuje kartę SD i wczytuje 5 ostatnich tras przy starcie
    void archiveCurrentTrip(MotoState& state, int durationMin, float distKm, float avgFuel, int avgSpeed);

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;

    bool onModeLongPress(MotoState& state) override;   // Przytrzymanie MODE archiwizuje bieżącą trasę do tabeli
    bool onActionLongPress(MotoState& state) override; // Przytrzymanie ACTION czyści historię tabeli

private:
    bool sdAvailable = false; // Informuje, czy karta microSD jest włożona i gotowa do zapisu
    bool isDirty     = true;  // Flaga wymuszająca przerysowanie tabeli tylko po zmianie danych

    void saveToSdCsv(const TripRecord& rec); // Dopisuje nową trasę jako wiersz w pliku /trasy.csv na karcie SD
    String formatDuration(int totalMinutes); // Zamienia minuty na czytelny format "02:15"
};
