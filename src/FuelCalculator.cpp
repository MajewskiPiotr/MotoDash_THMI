#include "FuelCalculator.h"
#include "Config.h"
#include <Arduino.h>
#include <Preferences.h> // Wbudowana biblioteka ESP32 do nieulotnej pamięci Flash (NVS)

// Wczytuje wszystkie zapisane liczniki z pamięci Flash zaraz po przekręceniu kluczyka w stacyjce
void FuelCalculator::init(MotoState& state) {
    lastCalcMs = millis();
    lastSaveMs = millis();

    Preferences prefs;
    // Otwiera przestrzeń nazw "motodash" w trybie tylko do odczytu (true)
    if (prefs.begin("motodash", true)) {
        // 1. Licznik bieżącego baku (Ekran 1)
        tankKm                  = prefs.getFloat("tankKm", 0.0f);   // Dystans od tankowania [km]
        tankFuelUsedL           = prefs.getFloat("tankUsed", 0.0f); // Paliwo zużyte od tankowania [L]
        state.overallAvgFuel100 = prefs.getFloat("tankAvg", 5.1f);  // Średnie spalanie z obecnego baku [L/100km]

        // 2. Licznik bieżącej wycieczki (Ekran 3)
        state.tripKm            = prefs.getFloat("tripKm", 0.0f);   // Dystans bieżącej wycieczki [km]
        tripFuelUsedL           = prefs.getFloat("tripFuel", 0.0f); // Paliwo zużyte na wycieczce [L]
        state.tripAvgFuel100    = prefs.getFloat("tripAvg", 5.1f);  // Średnie spalanie z wycieczki [L/100km]

        prefs.end(); // Zamyka dostęp do pamięci Flash
    }

    // Wylicza paliwo w baku i pozostały zasięg na podstawie średniego spalania ogólnego
    updateTankState(state);

    Serial.println("[FuelCalculator] Wczytano: Bak=" + String(tankKm, 1) + "km (" +
                   String(state.overallAvgFuel100, 1) + "L), Wycieczka=" +
                   String(state.tripKm, 1) + "km (" + String(state.tripAvgFuel100, 1) + "L)");
}

// Główna pętla obliczeniowa wywoływana w loop()
void FuelCalculator::update(MotoState& state) {
    unsigned long now = millis();
    float dtSec = (now - lastCalcMs) / 1000.0f; // Czas [s], jaki upłynął od poprzedniej próbki
    if (dtSec < 0.25f) return;                  // Przeliczamy maksymalnie 4 razy na sekundę (co 250 ms)
    lastCalcMs = now;

    // 1. Obliczenie chwilowego przepływu paliwa w litrach na godzinę [L/h] metodą Speed-Density
    float fuelLh = 0.0f;
    if (state.rpm > 2200 && state.mapKpa < 25) {
        // Odcięcie wtrysku (DFCO) podczas hamowania silnikiem przy zamkniętej manetce gazu
        fuelLh = 0.0f;
    } else if (state.rpm > 500) {
        // Obliczenie masy zasysanego powietrza (MAF) dla silnika 4-suwowego Benelli TRK 702 (0.698 L)
        float kelvin = state.iatTempC + 273.15f;
        float imap   = (state.rpm * (float)state.mapKpa) / kelvin;
        float maf    = (imap / 120.0f) * Config::VOLUMETRIC_EFF * Config::ENGINE_DISPLACEMENT * (28.97f / 8.314f);

        // Przeliczenie powietrza na paliwo (AFR 14.7, gęstość 745 g/L) z uwzględnieniem kalibracji z Ekranu 4
        fuelLh = ((maf * 3600.0f) / (14.7f * 745.0f)) * state.fuelCalFactor;
    }

    // 2. Przyrost przejechanego dystansu i zużytego paliwa w kroku czasowym dtSec
    float distStep = (state.speedKmh / 3600.0f) * dtSec;
    float fuelStep = (fuelLh / 3600.0f) * dtSec;

    // Dodajemy ten sam przyrost jednocześnie do licznika baku oraz do licznika bieżącej wycieczki
    tankKm        += distStep;
    tankFuelUsedL += fuelStep;

    state.tripKm  += distStep;
    tripFuelUsedL += fuelStep;

    // 3. Spalanie chwilowe: L/100km podczas jazdy lub L/h na postoju (<= 3 km/h)
    state.instFuel100 = (state.speedKmh > 3) ? (fuelLh / state.speedKmh) * 100.0f : fuelLh;

    // 4. Aktualizacja średniego spalania z obecnego baku (po przejechaniu min. 200 m od tankowania)
    if (tankKm > 0.2f) {
        state.overallAvgFuel100 = (tankFuelUsedL / tankKm) * 100.0f;
    }

    // 5. Aktualizacja średniego spalania z bieżącej wycieczki (po przejechaniu min. 200 m na wycieczce)
    if (state.tripKm > 0.2f) {
        state.tripAvgFuel100 = (tripFuelUsedL / state.tripKm) * 100.0f;
    }

    // 6. Obliczenie paliwa w baku i pozostałego zasięgu
    updateTankState(state);

    // 7. Inteligentny zapis do nieulotnej pamięci Flash (chroni komórki pamięci przed zużyciem)
    bool isMoving = (state.speedKmh > 3);

    // Zapis następuje w chwili zatrzymania motocykla (wasMoving && !isMoving)
    // oraz profilaktycznie co 5 minut (300 000 ms) podczas ciągłej jazdy w trasie
    if ((wasMoving && !isMoving) || (isMoving && (now - lastSaveMs > 300000))) {
        saveToNvs(state);
        lastSaveMs = now;
    }
    wasMoving = isMoving;
}

// Zeruje zużyte paliwo oraz dystans od tankowania (odnawia zasięg i resetuje średnie spalanie na Ekranie 1)
// Nie rusza licznika bieżącej wycieczki (state.tripKm i state.tripAvgFuel100)!
void FuelCalculator::resetRefuel(MotoState& state) {
    tankFuelUsedL = 0.0f; // Zeruje zużyte litry z baku
    tankKm        = 0.0f; // Zeruje kilometry od tankowania (nowe średnie spalanie zacznie liczyć się od zera)
    updateTankState(state); // Pełny bak -> przelicza paliwo w baku i zasięg

    saveToNvs(state);
    Serial.println("[FuelCalculator] Tankowanie: zresetowano zasieg i srednie spalanie z baku!");
}

// Dolewa konkretną ilość paliwa (tankowanie częściowe). Nadmiar ponad pojemność baku jest ignorowany.
void FuelCalculator::addFuel(MotoState& state, float liters) {
    if (liters <= 0.0f) return;
    // Jeśli bak był "przepełniony" w obliczeniach (zużyto więcej niż pojemność), startujemy od pustego
    tankFuelUsedL = min(tankFuelUsedL, state.tankCapacityL);
    tankFuelUsedL = max(0.0f, tankFuelUsedL - liters);
    updateTankState(state);
    saveToNvs(state);
    Serial.println("[FuelCalculator] Dolano " + String(liters, 1) + " L -> w baku " +
                   String(state.fuelInTankL, 1) + " L");
}

// Wspólne przeliczenie ilości paliwa w baku i zasięgu (używane po każdej zmianie licznika baku)
void FuelCalculator::updateTankState(MotoState& state) {
    state.fuelInTankL = max(0.0f, state.tankCapacityL - tankFuelUsedL);
    state.rangeKm = (state.overallAvgFuel100 > 0.5f)
                    ? (int)((state.fuelInTankL / state.overallAvgFuel100) * 100.0f)
                    : 0;
}

// Zeruje licznik bieżącej wycieczki (wywoływane przy archiwizacji zakończonej wycieczki do historii CSV)
void FuelCalculator::resetCurrentTrip(MotoState& state) {
    state.tripKm         = 0.0f;
    tripFuelUsedL        = 0.0f;
    state.tripAvgFuel100 = state.overallAvgFuel100; // Na starcie nowej wycieczki przyjmuje średnią z baku
    saveToNvs(state);
    Serial.println("[FuelCalculator] Wyzerowano licznik pod nowa wycieczke!");
}

// Prywatna metoda zapisująca komplet liczników w nieulotnej pamięci Flash (NVS)
void FuelCalculator::saveToNvs(const MotoState& state) {
    Preferences prefs;
    // Otwiera pamięć w trybie zapisu (false)
    if (prefs.begin("motodash", false)) {
        prefs.putFloat("tankKm", tankKm);
        prefs.putFloat("tankUsed", tankFuelUsedL);
        prefs.putFloat("tankAvg", state.overallAvgFuel100);

        prefs.putFloat("tripKm", state.tripKm);
        prefs.putFloat("tripFuel", tripFuelUsedL);
        prefs.putFloat("tripAvg", state.tripAvgFuel100);

        prefs.end();
        Serial.println("[FuelCalculator] Zapisano liczniki baku i wycieczki do Flash");
    }
}
