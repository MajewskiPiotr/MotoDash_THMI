#pragma once
#include <Arduino.h>
#include "MotoState.h"

// Historia tankowań (wymaganie 2): okresy "od pełna do pełna", spalanie z dystrybutora
// a szacunek komputera, oraz samoczynne ustawianie mnożnika kalibracji.
//
// - "Dolej" dopisuje dolewkę do bieżącego okresu (bez osobnego pomiaru).
// - "Do pełna" zamyka okres: przeliczenie = (litry z dystrybutora + dolewki) / km okresu.
// - Mnożnik = średnia ważona litrami z 3 ostatnich ważnych baków; bak z różnicą > 30% jest pomijany.
// Ostatnie baki trzymane są w pamięci Flash (NVS), a z kartą SD każdy bak dopisuje się też do /tankowania.csv.

enum RefuelType : uint8_t { REFUEL_FULL = 0, REFUEL_TOPUP = 1, REFUEL_FULL_NO_PUMP = 2 };
enum TankStatus : uint8_t { TANK_OK = 0, TANK_SKIPPED = 1, TANK_NO_MEASURE = 2 };

constexpr uint8_t REFUEL_MAX_EVENTS = 6;  // Tankowań w jednym okresie (dolewki + "do pełna")
constexpr uint8_t REFUEL_MAX_TANKS  = 10; // Ile ostatnich baków pamiętamy w Flash
constexpr uint8_t REFUEL_CAL_TANKS  = 3;  // Z ilu ostatnich ważnych baków liczymy mnożnik

struct RefuelEvent {
    uint32_t time;   // Czas UNIX (0 = nieznany - zegar nieustawiony)
    float    odoKm;  // Stan licznika MotoDash [km]
    float    liters; // Wlane litry
    uint8_t  type;   // RefuelType
};

struct TankRecord {
    uint16_t    n            = 0;    // Numer baku
    uint32_t    startTime    = 0, endTime = 0;
    float       startOdo     = 0, endOdo = 0;
    float       km           = 0;    // Km okresu
    float       estL         = 0;    // Paliwo spalone wg komputera (szacunek)
    float       realL        = 0;    // Dystrybutor + dolewki (0 = bez pomiaru)
    float       factorBefore = 1.0f, factorAfter = 1.0f;
    uint8_t     status       = TANK_NO_MEASURE;
    uint8_t     nEv          = 0;
    float       topL         = 0;    // Suma dolewek w okresie [L] (liczona osobno - lista zdarzeń jest ograniczona)
    uint8_t     topN         = 0;    // Liczba dolewek w okresie
    RefuelEvent ev[REFUEL_MAX_EVENTS]; // Ostatnie tankowania okresu (do raportu)
};

// Wynik zamknięcia okresu - do krótkiej informacji na ekranie po tankowaniu
struct RefuelResult {
    uint8_t status  = TANK_NO_MEASURE;
    float   oldF    = 1.0f;
    float   newF    = 1.0f;
    float   diffPct = 0.0f;  // (przeliczenie - szacunek) / szacunek * 100
};

class RefuelLog {
public:
    void init(float odoKm);              // Wczytuje historię z Flash; odoKm = obecny stan licznika
    void setSdAvailable(bool sd) { sdAvailable = sd; }

    // Dolewka ("Dolej") - dopisywana do bieżącego okresu
    void addTopUp(float liters, float odoKm);

    // Tankowanie do pełna. pumpedL = litry z dystrybutora (0 = "Pomiń" / bez pomiaru).
    // km i estL = dystans i paliwo wg komputera od poprzedniego "do pełna"; measurable = czy okres nadaje się do pomiaru.
    // Przy ważnym pomiarze ustawia state.fuelCalFactor i zapisuje go w Flash.
    RefuelResult closeFullTank(MotoState& state, float pumpedL, float km, float estL, float odoKm, bool measurable);

    // Dane do raportu na stronie Wi-Fi
    uint8_t           tankCount() const        { return count; }
    const TankRecord& tankAt(uint8_t i) const  { return tanks[i]; } // 0 = najnowszy
    const TankRecord& openPeriod() const       { return current; }
    bool              hasSd() const            { return sdAvailable; }

    static uint32_t now(); // Czas UNIX albo 0, jeśli zegar nie został ustawiony (telefon / GPS)

private:
    TankRecord tanks[REFUEL_MAX_TANKS]; // Zamknięte baki, [0] = najnowszy
    uint8_t    count = 0;
    TankRecord current;                 // Bieżący (otwarty) okres
    bool       sdAvailable = false;

    void addEvent(TankRecord& t, float liters, float odoKm, uint8_t type);
    void startNewPeriod(float odoKm, uint16_t n);
    float computeFactor(float fallback) const; // Mnożnik z ostatnich ważnych baków
    void save();
    void appendCsv(const TankRecord& t);
};
