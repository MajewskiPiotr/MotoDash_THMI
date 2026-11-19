#include "FuelCalculator.h"
#include "Config.h"
#include <Arduino.h>
#include <Preferences.h> // Wbudowana biblioteka ESP32 do nieulotnej pamięci Flash (NVS)

// Wczytuje wszystkie liczniki z pamięci Flash zaraz po przekręceniu kluczyka
void FuelCalculator::init(MotoState& state) {
    lastCalcMs = millis();
    lastSaveMs = millis();

    Preferences prefs;
    // Otwiera przestrzeń "motodash" w trybie odczytu (true)
    if (prefs.begin("motodash", true)) {
        // 1. Licznik baku
        tankFuelUsedL           = prefs.getFloat("tankUsed", 0.0f);

        // 2. Licznik bieżącej wycieczki
        state.tripKm            = prefs.getFloat("tripKm", 0.0f);
        tripFuelUsedL           = prefs.getFloat("tripFuel", 0.0f);
        state.tripAvgFuel100    = prefs.getFloat("tripAvg", 5.1f);

        // 3. Licznik ogólny (długoterminowy)
        overallKm               = prefs.getFloat("ovlKm", 0.0f);
        overallFuelUsedL        = prefs.getFloat("ovlFuel", 0.0f);
        state.overallAvgFuel100 = prefs.getFloat("ovlAvg", 5.1f);

        prefs.end();
    }

    // Wylicza pozostały zasięg na podstawie stanu baku i średniego spalania ogólnego
    float remainingL = max(0.0f, state.tankCapacityL - tankFuelUsedL);
    state.rangeKm = (state.overallAvgFuel100 > 0.5f)
                    ? (int)((remainingL / state.overallAvgFuel100) * 100.0f)
                    : 0;

    Serial.println("[FuelCalculator] Wczytano: Trip=" + String(state.tripKm, 1) +
                   "km (" + String(state.tripAvgFuel100, 1) + "L), Ogolne=" +
                   String(state.overallAvgFuel100, 1) + "L");
}

// Główna pętla obliczeniowa wywoływana w loop()
void FuelCalculator::update(MotoState& state) {
    unsigned long now = millis();
    float dtSec = (now - lastCalcMs) / 1000.0f; // Czas [s] od ostatniej próbki
    if (dtSec < 0.25f) return;                  // Liczymy 4 razy na sekundę
    lastCalcMs = now;

    // 1. Obliczenie chwilowego przepływu paliwa w L/h metodą Speed-Density
    float fuelLh = 0.0f;
    if (state.rpm > 2200 && state.mapKpa < 25) {
        fuelLh = 0.0f; // Odcięcie wtrysku przy hamowaniu silnikiem
    } else if (state.rpm > 500) {
        float kelvin = state.iatTempC + 273.15f;
        float imap   = (state.rpm * (float)state.mapKpa) / kelvin;
        float maf    = (imap / 120.0f) * Config::VOLUMETRIC_EFF * Config::ENGINE_DISPLACEMENT * (28.97f / 8.314f);
        fuelLh = ((maf * 3600.0f) / (14.7f * 745.0f)) * state.fuelCalFactor;
    }

    // 2. Przyrost dystansu i zużytego paliwa w kroku dtSec
    float distStep = (state.speedKmh / 3600.0f) * dtSec;
    float fuelStep = (fuelLh / 3600.0f) * dtSec;

    // Dodajemy przyrost jednocześnie do wszystkich trzech liczników
    tankFuelUsedL    += fuelStep;

    state.tripKm     += distStep;
    tripFuelUsedL    += fuelStep;

    overallKm        += distStep;
    overallFuelUsedL += fuelStep;

    // 3. Spalanie chwilowe: L/100km w ruchu lub L/h na postoju
    state.instFuel100 = (state.speedKmh > 3) ? (fuelLh / state.speedKmh) * 100.0f : fuelLh;

    // 4. Aktualizacja średniego spalania z wycieczki (po przejechaniu min. 200 m na wycieczce)
    if (state.tripKm > 0.2f) {
        state.tripAvgFuel100 = (tripFuelUsedL / state.tripKm) * 100.0f;
    }

    // 5. Aktualizacja średniego spalania ogólnego (po przejechaniu min. 200 m ogółem)
    if (overallKm > 0.2f) {
        state.overallAvgFuel100 = (overallFuelUsedL / overallKm) * 100.0f;
    }

    // 6. Obliczenie pozostałego zasięgu na baku
    float remainingL = max(0.0f, state.tankCapacityL - tankFuelUsedL);
    state.rangeKm = (state.overallAvgFuel100 > 0.5f)
                    ? (int)((remainingL / state.overallAvgFuel100) * 100.0f)
                    : 0;

    // 7. Inteligentny zapis do pamięci Flash (przy zatrzymaniu motocykla lub co 5 minut w trasie)
    bool isMoving = (state.speedKmh > 3);
    if ((wasMoving && !isMoving) || (isMoving && (now - lastSaveMs > 300000))) {
        saveToNvs(state);
        lastSaveMs = now;
    }
    wasMoving = isMoving;
}

// Zeruje wyłącznie zużyte paliwo z obecnego baku (po zatankowaniu do pełna)
void FuelCalculator::resetRefuel(MotoState& state) {
    tankFuelUsedL = 0.0f;
    state.rangeKm = (state.overallAvgFuel100 > 0.5f)
                    ? (int)((state.tankCapacityL / state.overallAvgFuel100) * 100.0f)
                    : 0;
    saveToNvs(state);
    Serial.println("[FuelCalculator] Zatankowano do pelna - odnowiono zasieg!");
}

// Zeruje licznik bieżącej wycieczki (wywoływane przy zamknięciu wycieczki do historii CSV)
void FuelCalculator::resetCurrentTrip(MotoState& state) {
    state.tripKm         = 0.0f;
    tripFuelUsedL        = 0.0f;
    state.tripAvgFuel100 = state.overallAvgFuel100; // Na starcie nowej wycieczki przyjmuje średnią ogólną
    saveToNvs(state);
    Serial.println("[FuelCalculator] Wyzerowano licznik pod nowa wycieczke!");
}

// Zeruje licznik ogólny (długoterminowy)
void FuelCalculator::resetOverall(MotoState& state) {
    overallKm               = 0.0f;
    overallFuelUsedL        = 0.0f;
    state.overallAvgFuel100 = 5.1f;
    saveToNvs(state);
    Serial.println("[FuelCalculator] Wyzerowano spalanie ogolne!");
}

// Zapisuje komplet wszystkich liczników w nieulotnej pamięci Flash (NVS)
void FuelCalculator::saveToNvs(const MotoState& state) {
    Preferences prefs;
    if (prefs.begin("motodash", false)) {
        prefs.putFloat("tankUsed", tankFuelUsedL);

        prefs.putFloat("tripKm", state.tripKm);
        prefs.putFloat("tripFuel", tripFuelUsedL);
        prefs.putFloat("tripAvg", state.tripAvgFuel100);

        prefs.putFloat("ovlKm", overallKm);
        prefs.putFloat("ovlFuel", overallFuelUsedL);
        prefs.putFloat("ovlAvg", state.overallAvgFuel100);

        prefs.end();
        Serial.println("[FuelCalculator] Zapisano wszystkie liczniki do Flash");
    }
}
