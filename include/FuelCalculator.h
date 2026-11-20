#pragma once
#include "MotoState.h"

// Serwis wyliczający spalanie chwilowe, średnie ogólne, średnie z wycieczki oraz zasięg.
// Utrwala wszystkie liczniki w nieulotnej pamięci Flash (NVS), aby przetrwały postoje.
class FuelCalculator {
public:
    void init(MotoState& state);            // Wczytuje wszystkie liczniki z pamięci Flash przy starcie
    void update(MotoState& state);          // Przelicza spalanie i zapisuje stan na postojach / co 5 min
    void resetRefuel(MotoState& state);     // Zeruje paliwo w baku po tankowaniu (nie kasuje wycieczki!)
    void resetCurrentTrip(MotoState& state);// Zeruje licznik bieżącej wycieczki (po zapisaniu jej do historii)
    void resetOverall(MotoState& state);    // Zeruje licznik ogólny (długoterminowy)

private:
    unsigned long lastCalcMs     = 0;     // Znacznik czasu ostatniego obliczenia [ms]
    unsigned long lastSaveMs     = 0;     // Znacznik czasu ostatniego zapisu do Flash [ms]
    bool          wasMoving      = false; // Flaga wykrywająca moment zatrzymania motocykla

    // 1. Licznik bieżącego baku (zerowany przy tankowaniu do pełna -> Ekran 1)
    float tankKm                 = 0.0f;  // Dystans przejechany od ostatniego tankowania [km]
    float tankFuelUsedL          = 0.0f;  // Paliwo zużyte od ostatniego tankowania [L]

    // 2. Licznik bieżącej wycieczki (przetrwa postoje na kawę -> Ekran 3)
    float tripFuelUsedL          = 0.0f;  // Paliwo zużyte podczas bieżącej wycieczki [L]


    // 3. Licznik ogólny (długoterminowy)
    float overallKm              = 0.0f;  // Całkowity dystans zliczony przez komputer [km]
    float overallFuelUsedL       = 0.0f;  // Całkowite zużyte paliwo [L]

    void saveToNvs(const MotoState& state); // Zapisuje komplet liczników do nieulotnej pamięci Flash
};
