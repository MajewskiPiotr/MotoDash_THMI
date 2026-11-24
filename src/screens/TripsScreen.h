#pragma once
#include "Screen.h"
#include "FuelCalculator.h"

// Ekran prezentujący tabelę 5 ostatnich zakończonych tras (czas, dystans, średnie spalanie, średnia prędkość)
// wraz z odczytem/zapisem do pliku CSV na karcie microSD.
class TripsScreen : public Screen {
public:
    explicit TripsScreen(FuelCalculator* fuelCalc) : fuelCalculator(fuelCalc) {}

    void initSdAndLoad(MotoState& state); // Inicjuje kartę SD i wczytuje 5 ostatnich tras przy starcie
    void archiveCurrentTrip(MotoState& state, int durationMin, float distKm, float avgFuel, int avgSpeed);

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;

    // Przytrzymanie przycisku "Zakończ trasę" zapisuje bieżącą trasę i zeruje licznik wycieczki,
    // przytrzymanie "Wyczyść" czyści tabelę
    bool onLongTouch(MotoState& state, int x, int y) override;

    bool hasSd() const { return sdAvailable; }         // Czy karta microSD działa (np. do pobrania CSV przez Wi-Fi)

private:
    FuelCalculator* fuelCalculator; // Zeruje licznik wycieczki po jej zapisaniu do tabeli
    bool sdAvailable = false; // Informuje, czy karta microSD jest włożona i gotowa do zapisu
    bool isDirty     = true;  // Flaga wymuszająca przerysowanie tabeli tylko po zmianie danych

    void saveToSdCsv(const TripRecord& rec); // Dopisuje nową trasę jako wiersz w pliku /trasy.csv na karcie SD
    void finishTrip(MotoState& state);       // Zapisuje bieżącą trasę do tabeli i zeruje licznik wycieczki
    String formatDuration(int totalMinutes); // Zamienia minuty na czytelny format "02:15"
};
