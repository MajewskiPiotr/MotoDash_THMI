#pragma once
#include "MotoState.h"

// Serwis wyliczający spalanie chwilowe, średnie ogólne, średnie z wycieczki oraz zasięg.
// Utrwala wszystkie liczniki w nieulotnej pamięci Flash (NVS), aby przetrwały postoje.
class FuelCalculator {
public:
    void init(MotoState& state);            // Wczytuje wszystkie liczniki z pamięci Flash przy starcie
    void update(MotoState& state);          // Przelicza spalanie i zapisuje stan na postojach / co 5 min
    void resetRefuel(MotoState& state);     // Zeruje paliwo w baku po tankowaniu (nie kasuje wycieczki!)
    void addFuel(MotoState& state, float liters); // Dolewa konkretną ilość paliwa [L] (tankowanie częściowe)
    void resetCurrentTrip(MotoState& state);// Zeruje licznik bieżącej wycieczki (po zapisaniu jej do historii)

    // --- DANE DLA HISTORII TANKOWAŃ I KALIBRACJI (RefuelLog) ---
    bool  canCalibrate() const;                // Czy od poprzedniego "do pełna" spalono dość paliwa do pomiaru
    float expectedRefillL() const { return tankFuelUsedL; } // Ile litrów wg komputera powinno wejść do pełna
    float tankDistanceKm() const  { return tankKm; }        // Km od ostatniego tankowania do pełna
    float tankBurnedFuelL() const { return tankBurnedL; }   // Paliwo spalone wg komputera od tankowania do pełna [L]
    float odometerKm() const      { return odoKm; }         // Całkowity przebieg zliczony przez MotoDash [km]

private:
    unsigned long lastCalcMs     = 0;     // Znacznik czasu ostatniego obliczenia [ms]
    unsigned long lastSaveMs     = 0;     // Znacznik czasu ostatniego zapisu do Flash [ms]
    bool          wasMoving      = false; // Flaga wykrywająca moment zatrzymania motocykla

    // 1. Licznik bieżącego baku (zerowany przy tankowaniu do pełna -> Ekran 1)
    float tankKm                 = 0.0f;  // Dystans przejechany od ostatniego tankowania [km]
    float tankFuelUsedL          = 0.0f;  // Ubytek paliwa w baku [L] (pojemność - stan baku; zmniejsza go dolewanie)
    float tankBurnedL            = 0.0f;  // Paliwo faktycznie spalone od tankowania do pełna [L] (do średniej spalania)
    float tankAddedL             = 0.0f;  // Suma dolewek ("DOLEJ") od ostatniego tankowania do pełna [L]
    bool  tankFromFull           = false; // Czy liczniki baku liczą się od prawdziwego tankowania do pełna (warunek kalibracji)

    // Całkowity przebieg zliczony przez komputer (do historii tankowań)
    float odoKm                  = 0.0f;

    // 2. Licznik bieżącej wycieczki (przetrwa postoje na kawę -> Ekran 3)
    float tripFuelUsedL          = 0.0f;  // Paliwo zużyte podczas bieżącej wycieczki [L]

    void saveToNvs(const MotoState& state); // Zapisuje komplet liczników do nieulotnej pamięci Flash
    void updateTankState(MotoState& state); // Przelicza paliwo w baku i zasięg na podstawie tankFuelUsedL
};
