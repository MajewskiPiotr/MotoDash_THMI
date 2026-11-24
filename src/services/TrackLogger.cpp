#include "TrackLogger.h"
#include "Config.h"
#include "GpsService.h"
#include <SD_MMC.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

const char* TrackLogger::DIR = "/trasy";

// Zakończenie pliku - dopisywane po każdym zapisie i nadpisywane przy następnym
static const char TAIL[] = "</trkseg></trk>\n</gpx>\n";
static const size_t TAIL_LEN = sizeof(TAIL) - 1;

void TrackLogger::init(bool sdAvailable) {
    sd = sdAvailable;
    if (sd && !SD_MMC.exists(DIR)) SD_MMC.mkdir(DIR);
}

void TrackLogger::update(MotoState& s) {
    if (s.gpsStatus != 2 || s.gpsFixCount == lastFixCount) {
        if (s.gpsStatus != 2 && s.trackStatus == 1) s.trackStatus = 0;
        return;
    }
    lastFixCount = s.gpsFixCount;
    time_t now = time(nullptr);
    if (now < 1700000000) return; // Zegar jeszcze nieustawiony

    curLat = s.gpsLat; curLon = s.gpsLon; curAlt = s.altValid ? s.altitudeM : s.gpsAltM; havePos = true;

    if (!sd) { s.trackStatus = 2; return; }
    if (!started) startTrip(s, now);
    if (!started) { s.trackStatus = 2; return; } // Nie da się utworzyć pliku
    s.trackStatus = 1;

    // --- Postoje: prędkość < 3 km/h przez >= 5 min, zapis przy ruszeniu (znamy wtedy czas postoju) ---
    if (s.gpsSpeedKmh < Config::STOP_SPEED_KMH) {
        if (!slow) { slow = true; slowSince = now; stopLat = curLat; stopLon = curLon; }
    } else if (s.gpsSpeedKmh > Config::STOP_SPEED_KMH + 2.0f && slow) { // +2 km/h histerezy na szum GPS
        slow = false;
        long dur = (long)(now - slowSince);
        if (dur >= Config::STOP_MIN_S) writeWaypoint(stopLat, stopLon, slowSince, dur, "Postój", "");
    }

    // --- Punkty trasy ---
    float moved = havePt ? distanceM(ptLat, ptLon, curLat, curLon) : 1e9f;
    long  since = havePt ? (long)(now - ptTime) : 1000000L;
    if ((since >= Config::GPX_INTERVAL_S && moved >= Config::GPX_MIN_MOVE_M) || moved >= Config::GPX_MAX_GAP_M) {
        char xml[160];
        snprintf(xml, sizeof(xml), "<trkpt lat=\"%.6f\" lon=\"%.6f\"><ele>%.0f</ele><time>%s</time></trkpt>\n",
                 curLat, curLon, curAlt, isoTime(now).c_str());
        if (appendXml(xml)) { ptLat = curLat; ptLon = curLon; ptTime = now; havePt = true; }
    }
}

// Wybór pliku: kontynuacja ostatniej trasy (krótka przerwa tego samego dnia) albo nowy plik
void TrackLogger::startTrip(const MotoState& s, time_t now) {
    struct tm lt;
    localtime_r(&now, &lt);

    // Najnowszy plik w /trasy (nazwy z datą sortują się chronologicznie)
    String newest;
    fs::File dir = SD_MMC.open(DIR);
    if (dir && dir.isDirectory()) {
        for (fs::File e = dir.openNextFile(); e; e = dir.openNextFile()) {
            String n = e.name();
            int slash = n.lastIndexOf('/');
            if (slash >= 0) n = n.substring(slash + 1);
            if (n.endsWith(".gpx") && n > newest) newest = n;
            e.close();
        }
        dir.close();
    }

    if (newest.length()) {
        String f = String(DIR) + "/" + newest;
        double lat, lon; time_t last;
        if (readLastPoint(f, lat, lon, last)) {
            struct tm pt;
            localtime_r(&last, &pt);
            long gap = (long)(now - last);
            bool sameDay = pt.tm_year == lt.tm_year && pt.tm_yday == lt.tm_yday;
            if (gap >= 0 && gap < Config::TRIP_GAP_S && sameDay) {
                path = f; started = true;
                ptLat = lat; ptLon = lon; ptTime = last; havePt = true;
                Serial.println("[Trasa] Ciag dalszy wycieczki: " + path + " (przerwa " + String(gap / 60) + " min)");
                if (gap >= Config::STOP_MIN_S) writeWaypoint(lat, lon, last, gap, "Postój", "zapłon wyłączony");
                return;
            }
        }
    }

    char name[40];
    snprintf(name, sizeof(name), "%s/%04d-%02d-%02d_%02d%02d.gpx", DIR,
             lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min);
    fs::File f = SD_MMC.open(name, FILE_WRITE);
    if (!f) { Serial.println("[Trasa] Nie mozna utworzyc pliku " + String(name)); return; }
    char title[24];
    snprintf(title, sizeof(title), "%02d.%02d.%04d %02d:%02d", lt.tm_mday, lt.tm_mon + 1, lt.tm_year + 1900, lt.tm_hour, lt.tm_min);
    f.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<gpx version=\"1.1\" creator=\"MotoDash\" xmlns=\"http://www.topografix.com/GPX/1/1\">\n"
            "<trk><name>MotoDash ");
    f.print(title);
    f.print("</name><trkseg>\n");
    f.print(TAIL);
    f.close();
    path = name; started = true; havePt = false;
    Serial.println("[Trasa] Nowa wycieczka: " + path);
}

// Dopisuje fragment przed zakończeniem pliku (nadpisuje stare zakończenie i dodaje nowe)
bool TrackLogger::appendXml(const String& xml) {
    fs::File f = SD_MMC.open(path.c_str(), "r+");
    if (!f) return false;
    size_t size = f.size();
    if (size < TAIL_LEN || !f.seek(size - TAIL_LEN)) { f.close(); return false; }
    f.print(xml);
    f.print(TAIL);
    f.close();
    return true;
}

// Waypoint musi leżeć poza <trk>, więc zamykamy odcinek, wstawiamy punkt i otwieramy kolejny odcinek
void TrackLogger::writeWaypoint(double lat, double lon, time_t arrival, long durationS, const char* name, const String& desc) {
    struct tm lt;
    localtime_r(&arrival, &lt);
    char hm[8];
    snprintf(hm, sizeof(hm), "%02d:%02d", lt.tm_hour, lt.tm_min);
    String d = desc;
    if (durationS > 0) {
        String dur;
        if (durationS >= 3600) dur = String(durationS / 3600) + " h " + String((durationS % 3600) / 60) + " min";
        else                   dur = String(durationS / 60) + " min";
        String extra = d;
        d = "od " + String(hm) + ", " + dur;
        if (extra.length()) d += ", " + extra;
    } else if (!d.length()) {
        d = String(hm);
    } else {
        d = String(hm) + ", " + d;
    }
    char head[120];
    snprintf(head, sizeof(head), "</trkseg></trk>\n<wpt lat=\"%.6f\" lon=\"%.6f\"><time>%s</time><name>",
             lat, lon, isoTime(arrival).c_str());
    String xml = String(head) + name + " " + hm + "</name><desc>" + d + "</desc></wpt>\n<trk><trkseg>\n";
    if (appendXml(xml)) Serial.println("[Trasa] " + String(name) + ": " + d);
}

void TrackLogger::addWaypoint(const char* name, const String& desc) {
    if (!sd || !started || !havePos) return;
    writeWaypoint(curLat, curLon, time(nullptr), 0, name, desc);
}

// Czyta ostatni <trkpt> z końcówki pliku (lat, lon, czas)
bool TrackLogger::readLastPoint(const String& file, double& lat, double& lon, time_t& t) {
    fs::File f = SD_MMC.open(file.c_str(), FILE_READ);
    if (!f) return false;
    size_t size = f.size();
    size_t from = size > 600 ? size - 600 : 0;
    f.seek(from);
    char buf[601];
    size_t n = f.read((uint8_t*)buf, size - from);
    f.close();
    buf[n] = 0;

    // Ostatnie wystąpienie "<trkpt "
    const char* p = nullptr;
    for (const char* q = strstr(buf, "<trkpt "); q; q = strstr(q + 1, "<trkpt ")) p = q;
    if (!p) return false;
    const char* la = strstr(p, "lat=\"");
    const char* lo = strstr(p, "lon=\"");
    const char* tm = strstr(p, "<time>");
    if (!la || !lo || !tm) return false;
    lat = atof(la + 5);
    lon = atof(lo + 5);
    t = parseIso(tm + 6);
    return t > 0;
}

// Odległość po kuli ziemskiej (wzór haversine) [m]
float TrackLogger::distanceM(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0, k = M_PI / 180.0;
    double dLat = (lat2 - lat1) * k, dLon = (lon2 - lon1) * k;
    double a = sin(dLat / 2) * sin(dLat / 2) + cos(lat1 * k) * cos(lat2 * k) * sin(dLon / 2) * sin(dLon / 2);
    return (float)(2 * R * atan2(sqrt(a), sqrt(1 - a)));
}

String TrackLogger::isoTime(time_t t) {
    struct tm u;
    gmtime_r(&t, &u);
    char b[24];
    snprintf(b, sizeof(b), "%04d-%02d-%02dT%02d:%02d:%02dZ", u.tm_year + 1900, u.tm_mon + 1, u.tm_mday, u.tm_hour, u.tm_min, u.tm_sec);
    return String(b);
}

time_t TrackLogger::parseIso(const char* s) {
    int y, mo, d, h, mi, se;
    if (sscanf(s, "%4d-%2d-%2dT%2d:%2d:%2d", &y, &mo, &d, &h, &mi, &se) != 6) return 0;
    return GpsService::utcToEpoch(y, mo, d, h, mi, se);
}
