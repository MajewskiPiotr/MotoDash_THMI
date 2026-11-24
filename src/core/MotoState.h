#pragma once
#include <Arduino.h>
struct TripRecord {
    int   id          = 0;
    int   durationMin = 0;
    float distanceKm  = 0.0f;
    float avgFuel100  = 0.0f;
    int   avgSpeedKmh = 0;

    TripRecord() = default;
    TripRecord(int i, int dur, float dist, float fuel, int spd)
        : id(i), durationMin(dur), distanceKm(dist), avgFuel100(fuel), avgSpeedKmh(spd) {}
};

// Centralny obiekt stanu aplikacji (DTO). Przekazywany przez referencję między serwisami a ekranami.
struct MotoState {

        // --- HISTORIA 5 OSTATNICH TRAS (EKRAN ANALITYKI) ---
    static constexpr uint8_t MAX_HISTORY = 5;
    TripRecord history[MAX_HISTORY]; // Tablica 5 ostatnich tras (indeks 0 = najnowsza)
    int        historyCount = 0;     // Liczba zapisanych pozycji w tabeli (0..5)
    int        nextTripId   = 1;     // Licznik nadający kolejne numery trasom

    
    // --- DANE BIEŻĄCE Z ECU (CAN / OBD-II) ---
    int   rpm             = 0;     // Aktualne obroty silnika [obr./min]
    int   speedKmh        = 0;     // Aktualna prędkość motocykla [km/h]
    int   engineTempC     = 85;    // Temperatura płynu chłodzącego [°C]
    int   mapKpa          = 35;    // Ciśnienie bezwzględne w kolektorze dolotowym [kPa]
    int   iatTempC        = 20;    // Temperatura powietrza w dolocie [°C]
    float voltageV        = 14.2f; // Aktualne napięcie ładowania akumulatora [V]
    int   dtcCount        = 0;     // Liczba błędów zapisanych w sterowniku silnika

    // --- STATYSTYKI MOTOCYKLA (EKRAN 2) ---
    int   maxRpm          = 0;     // Najwyższe obroty osiągnięte od resetu
    int   maxSpeedKmh     = 0;     // Najwyższa prędkość osiągnięta od resetu [km/h]
    int   maxEngineTempC  = 0;     // Najwyższa temperatura silnika od resetu
    float minBootVoltageV = 14.2f; // Najniższy spadek napięcia (np. przy rozruchu)

    // --- DANE Z CZUJNIKA OTOCZENIA (GROVE I2C) ---
    float ambientTempC    = 18.5f; // Temperatura powietrza na zewnątrz [°C]
    float altitudeM       = 120.0f;// Aktualna wysokość n.p.m. z barometru [m]
    float pressureHpa     = 0.0f;  // Ciśnienie atmosferyczne z BME280 [hPa]
    bool  baroOk          = false; // Czy jest aktualny odczyt z barometru

    // --- DANE Z OPON (BLUETOOTH BLE TPMS) ---
    // Wartości mają sens tylko, gdy *TpmsOk == true (świeży odczyt z czujnika)
    bool  frontTpmsOk     = false; // Czy jest aktualny odczyt z czujnika przedniego
    float frontBar        = 0.0f;  // Ciśnienie w przedniej oponie [bar]
    int   frontTempC      = 0;     // Temperatura powietrza w przedniej oponie [°C]
    int   frontBattPct    = -1;    // Bateria czujnika przedniego [%] (-1 = nieznana)
    bool  rearTpmsOk      = false; // Czy jest aktualny odczyt z czujnika tylnego
    float rearBar         = 0.0f;  // Ciśnienie w tylnej oponie [bar]
    int   rearTempC       = 0;     // Temperatura powietrza w tylnej oponie [°C]
    int   rearBattPct     = -1;    // Bateria czujnika tylnego [%] (-1 = nieznana)
    
    // --- WYLICZENIA SPALANIA, DYSTANSU I ZASIĘGU ---
    float instFuel100       = 0.0f;  // Spalanie chwilowe [L/100km w ruchu lub L/h na postoju]
    float tankAvgFuel100 = 5.1f;  // Średnie spalanie od ostatniego tankowania do pełna [L/100km]
    float tripAvgFuel100    = 5.1f;  // Średnie spalanie z bieżącej wycieczki [L/100km]
    float tripKm            = 0.0f;  // Dystans bieżącej wycieczki [km]
    int   rangeKm           = 320;   // Szacowany zasięg na pozostałym paliwie w baku [km]
    float fuelInTankL       = 20.0f; // Szacowana ilość paliwa w baku [L] (pojemność - zużyte od tankowania)


    // --- USTAWIENIA EDYTOWALNE W KONFIGURACJI (EKRAN 4 - ZAPISYWANE W FLASH) ---
    float fuelCalFactor   = 1.00f; // Mnożnik kalibracji spalania (np. 1.04 = +4%)
    float tankCapacityL   = 20.0f; // Pojemność zbiornika paliwa [L]
    float minFrontBar     = 2.0f;  // Próg alarmu niskiego ciśnienia z przodu [bar]
    float minRearBar      = 2.2f;  // Próg alarmu niskiego ciśnienia z tyłu [bar]
    int   baseAltitudeM   = 100;   // Wysokość odniesienia do kalibracji barometru [m n.p.m.]

    // --- STAN INTERFEJSU UŻYTKOWNIKA ---
    bool nightBrightness  = false; // Flaga trybu nocnego podświetlenia
    bool forceRedraw      = true;  // Wymusza pełne przerysowanie tła przy zmianie widoku
};
