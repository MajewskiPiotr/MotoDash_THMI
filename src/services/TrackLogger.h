#pragma once
#include <Arduino.h>
#include "MotoState.h"

// Zapis trasy z GPS do plików GPX na karcie microSD (katalog /trasy, nazwa z datą: 2026-11-24_1432.gpx).
// - punkt co ~5 s (gdy motocykl się porusza), przy szybkiej jeździe co ~50 m,
// - postoje >= 5 min jako punkty (waypointy) z godziną przyjazdu i czasem postoju,
// - po wyłączeniu i włączeniu zapłonu w ciągu 2 h tego samego dnia trasa dopisuje się do tego samego pliku,
//   a przerwa zaznacza się jako postój; dłuższa przerwa albo nowy dzień = nowy plik.
// Plik jest domykany po każdym zapisie, więc nagłe wyłączenie zasilania go nie psuje.
class TrackLogger {
public:
    void init(bool sdAvailable);
    void update(MotoState& state);

    // Punkt specjalny w bieżącym miejscu (np. tankowanie); bez pozycji GPS nic nie robi
    void addWaypoint(const char* name, const String& desc);

    const String& currentFile() const { return path; }

    static const char* DIR; // "/trasy"

private:
    bool     sd       = false;
    bool     started  = false;  // Plik bieżącej wycieczki wybrany (po pierwszej pozycji GPS)
    String   path;
    uint32_t lastFixCount = 0;

    double   curLat = 0, curLon = 0;  float curAlt = 0; bool havePos = false;
    double   ptLat  = 0, ptLon  = 0;  time_t ptTime = 0; bool havePt = false; // Ostatni zapisany punkt

    // Wykrywanie postoju
    bool     slow = false;
    time_t   slowSince = 0;
    double   stopLat = 0, stopLon = 0;

    void   startTrip(const MotoState& s, time_t now);
    bool   appendXml(const String& xml);
    void   writeWaypoint(double lat, double lon, time_t arrival, long durationS, const char* name, const String& desc);
    bool   readLastPoint(const String& file, double& lat, double& lon, time_t& t);

    static float  distanceM(double lat1, double lon1, double lat2, double lon2);
    static String isoTime(time_t t);
    static time_t parseIso(const char* s);
};
