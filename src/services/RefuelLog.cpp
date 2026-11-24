#include "RefuelLog.h"
#include "TrackLogger.h"
#include "Config.h"
#include <Preferences.h>
#include <SD_MMC.h>
#include <time.h>

static const char*   NVS_NS      = "refuel"; // Osobna przestrzeń w Flash dla historii tankowań
static const uint8_t NVS_VERSION = 1;        // Zmiana układu struktur -> podnieść wersję (stare dane zostaną pominięte)

// ---------------------------------------------------------------------------
// Start / zapis
// ---------------------------------------------------------------------------

void RefuelLog::init(float odoKm) {
    // Czas polski z automatyczną zmianą letni/zimowy (do dat w pliku CSV)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    Preferences prefs;
    bool loaded = false;
    if (prefs.begin(NVS_NS, true)) {
        if (prefs.getUChar("ver", 0) == NVS_VERSION &&
            prefs.getBytesLength("tanks") == sizeof(tanks) &&
            prefs.getBytesLength("cur") == sizeof(current)) {
            count = min<uint8_t>(prefs.getUChar("cnt", 0), REFUEL_MAX_TANKS);
            prefs.getBytes("tanks", tanks, sizeof(tanks));
            prefs.getBytes("cur", &current, sizeof(current));
            loaded = true;
        }
        prefs.end();
    }
    if (!loaded) {
        count = 0;
        startNewPeriod(odoKm, 1);
    }
    Serial.println("[RefuelLog] Bakow w historii: " + String(count) + ", biezacy okres: bak " + String(current.n));
}

void RefuelLog::save() {
    Preferences prefs;
    if (prefs.begin(NVS_NS, false)) {
        prefs.putUChar("ver", NVS_VERSION);
        prefs.putUChar("cnt", count);
        prefs.putBytes("tanks", tanks, sizeof(tanks));
        prefs.putBytes("cur", &current, sizeof(current));
        prefs.end();
    }
}

uint32_t RefuelLog::now() {
    time_t t = time(nullptr);
    return (t > 1700000000) ? (uint32_t)t : 0; // Przed ustawieniem zegara ESP32 liczy od 1970 r.
}

// ---------------------------------------------------------------------------
// Tankowania
// ---------------------------------------------------------------------------

void RefuelLog::startNewPeriod(float odoKm, uint16_t n) {
    current           = TankRecord();
    current.n         = n;
    current.startOdo  = odoKm;
    current.startTime = now();
}

void RefuelLog::addEvent(TankRecord& t, float liters, float odoKm, uint8_t type) {
    if (t.nEv >= REFUEL_MAX_EVENTS) { // Lista pełna - usuwamy najstarsze zdarzenie (sumy są liczone osobno)
        for (uint8_t i = 1; i < REFUEL_MAX_EVENTS; i++) t.ev[i - 1] = t.ev[i];
        t.nEv = REFUEL_MAX_EVENTS - 1;
    }
    t.ev[t.nEv++] = { now(), odoKm, liters, type };

    // Miejsce tankowania na trasie GPX (gdy jest pozycja GPS i karta SD)
    if (track) {
        String desc = "bez pomiaru litrów";
        if (liters > 0.0f) desc = String(liters, 1) + " L";
        track->addWaypoint(type == REFUEL_TOPUP ? "Dolewka" : "Tankowanie do pełna", desc);
    }
}

void RefuelLog::addTopUp(float liters, float odoKm) {
    if (liters <= 0.0f) return;
    addEvent(current, liters, odoKm, REFUEL_TOPUP);
    current.topL += liters;
    current.topN++;
    save();
    Serial.println("[RefuelLog] Dolewka " + String(liters, 1) + " L w baku " + String(current.n));
}

RefuelResult RefuelLog::closeFullTank(MotoState& state, float pumpedL, float km, float estL, float odoKm, bool measurable) {
    RefuelResult r;
    r.oldF = r.newF = state.fuelCalFactor;

    TankRecord t   = current;
    t.endTime      = now();
    t.endOdo       = odoKm;
    t.km           = km;
    t.estL         = estL;
    t.factorBefore = state.fuelCalFactor;
    addEvent(t, pumpedL, odoKm, pumpedL > 0.0f ? REFUEL_FULL : REFUEL_FULL_NO_PUMP);

    // Pomiar: (dystrybutor + dolewki) w porównaniu z szacunkiem komputera
    if (measurable && pumpedL > 0.0f && estL > 0.1f) {
        t.realL   = pumpedL + t.topL;
        float ratio = t.realL / estL;
        r.diffPct = (ratio - 1.0f) * 100.0f;
        bool sane = ratio <= Config::CAL_MAX_RATIO && ratio >= 1.0f / Config::CAL_MAX_RATIO;
        t.status  = sane ? TANK_OK : TANK_SKIPPED;
    } else {
        t.status = TANK_NO_MEASURE;
    }

    // Nowy bak na początek historii
    for (int i = REFUEL_MAX_TANKS - 1; i > 0; i--) tanks[i] = tanks[i - 1];
    tanks[0] = t;
    if (count < REFUEL_MAX_TANKS) count++;

    // Mnożnik liczymy tylko po ważnym pomiarze
    if (t.status == TANK_OK) {
        float newF = computeFactor(r.oldF);
        state.fuelCalFactor   = newF;
        state.tankAvgFuel100 *= newF / r.oldF; // Średnia od razu w nowej skali
        r.newF = newF;
        Preferences prefs;
        if (prefs.begin("motodash", false)) {
            prefs.putFloat("fuelCal", newF); // Ten sam klucz co w Ustawieniach (Kalibracja spalania)
            prefs.end();
        }
    }
    tanks[0].factorAfter = state.fuelCalFactor;
    r.status = t.status;

    Serial.println("[RefuelLog] Bak " + String(t.n) + ": " + String(t.km, 0) + " km, szacunek " + String(t.estL, 1) +
                   " L, dystrybutor+dolewki " + String(t.realL, 1) + " L, status " + String(t.status) +
                   ", mnoznik " + String(r.oldF, 3) + " -> " + String(r.newF, 3));

    appendCsv(tanks[0]);
    startNewPeriod(odoKm, t.n + 1);
    save();
    return r;
}

// Mnożnik = suma litrów z dystrybutora / suma "surowych" szacunków (bez mnożnika) z ostatnich ważnych baków.
// Dzięki temu baki liczone przy różnych mnożnikach są porównywalne, a większe baki ważą więcej.
float RefuelLog::computeFactor(float fallback) const {
    float sumReal = 0, sumRaw = 0;
    uint8_t used = 0;
    for (uint8_t i = 0; i < count && used < REFUEL_CAL_TANKS; i++) {
        const TankRecord& t = tanks[i];
        if (t.status != TANK_OK || t.factorBefore <= 0.0f) continue;
        sumReal += t.realL;
        sumRaw  += t.estL / t.factorBefore;
        used++;
    }
    if (sumRaw <= 0.0f) return fallback;
    return constrain(sumReal / sumRaw, Config::CAL_FACTOR_MIN, Config::CAL_FACTOR_MAX);
}

// ---------------------------------------------------------------------------
// Plik CSV na karcie SD (pełna historia, bez limitu 10 baków)
// ---------------------------------------------------------------------------

static String fmtTime(uint32_t t) {
    if (!t) return "";
    time_t tt = t;
    struct tm lt;
    localtime_r(&tt, &lt);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &lt);
    return String(buf);
}

void RefuelLog::appendCsv(const TankRecord& t) {
    if (!sdAvailable) return;
    bool isNew = !SD_MMC.exists("/tankowania.csv");
    fs::File f = SD_MMC.open("/tankowania.csv", FILE_APPEND);
    if (!f) return;
    if (isNew) {
        f.println("Bak;Od;Do;Licznik_od_km;Licznik_do_km;Km;Litry_razem;Dolewki_szt;Dolewki_L;"
                  "Szacunek_L100;Przeliczenie_L100;Roznica_proc;Mnoznik_przed;Mnoznik_po;Status");
    }
    const char* st = t.status == TANK_OK ? "OK" : t.status == TANK_SKIPPED ? "pominiety" : "bez_pomiaru";
    float est100  = t.km > 0.1f ? t.estL / t.km * 100.0f : 0;
    float real100 = (t.km > 0.1f && t.realL > 0) ? t.realL / t.km * 100.0f : 0;
    float diff    = (t.realL > 0 && t.estL > 0) ? (t.realL / t.estL - 1.0f) * 100.0f : 0;
    f.printf("%u;%s;%s;%.1f;%.1f;%.1f;%.2f;%u;%.1f;%.2f;%.2f;%.1f;%.3f;%.3f;%s\n",
             t.n, fmtTime(t.startTime).c_str(), fmtTime(t.endTime).c_str(), t.startOdo, t.endOdo, t.km,
             t.realL, t.topN, t.topL, est100, real100, diff, t.factorBefore, t.factorAfter, st);
    f.close();
}
