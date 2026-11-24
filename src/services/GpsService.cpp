#include "GpsService.h"
#include "Config.h"
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

// Kolejność prób: najpierw typowa dla Air530Z (9600 bodów), w obu ustawieniach pinów
const GpsService::PortCfg GpsService::CONFIGS[] = {
    {Config::PIN_GPS_A, Config::PIN_GPS_B, 9600},   {Config::PIN_GPS_B, Config::PIN_GPS_A, 9600},
    {Config::PIN_GPS_A, Config::PIN_GPS_B, 115200}, {Config::PIN_GPS_B, Config::PIN_GPS_A, 115200},
    {Config::PIN_GPS_A, Config::PIN_GPS_B, 38400},  {Config::PIN_GPS_B, Config::PIN_GPS_A, 38400},
    {Config::PIN_GPS_A, Config::PIN_GPS_B, 4800},   {Config::PIN_GPS_B, Config::PIN_GPS_A, 4800},
};
const uint8_t GpsService::CONFIG_COUNT = sizeof(CONFIGS) / sizeof(CONFIGS[0]);

static const unsigned long PROBE_MS   = 2200; // Tyle czekamy na NMEA przy każdej konfiguracji (moduł nadaje co 1 s)
static const unsigned long LOST_MS    = 5000; // Brak danych przez tyle ms = moduł odłączony
static const unsigned long FIX_LOST_MS = 3000;

void GpsService::init() {
    // Czas polski z automatyczną zmianą letni/zimowy (zegar ESP32 trzyma UTC)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    applyConfig(0);
    Serial.println("[GPS] Szukam modulu GPS na IO17/IO18...");
}

void GpsService::applyConfig(uint8_t i) {
    cfgIdx = i;
    cfgSinceMs = millis();
    goodCount = 0;
    len = 0;
    Serial1.end();
    Serial1.setRxBufferSize(1024);
    // Tylko odbiór (TX = -1): do modułu nic nie wysyłamy, a drugi pin złącza zostaje nieaktywny
    Serial1.begin(CONFIGS[i].baud, SERIAL_8N1, CONFIGS[i].rx, -1);
}

void GpsService::update(MotoState& s) {
    while (Serial1.available()) {
        char c = (char)Serial1.read();
        if (c == '\n' || c == '\r') {
            if (len > 0) {
                buf[len] = 0;
                if (feedLine(buf, s)) {
                    lastGoodMs = millis();
                    if (!locked && ++goodCount >= 2) {
                        locked = true;
                        searchStartMs = lastSearchLogMs = millis();
                        Serial.printf("[GPS] Modul wykryty: RX=IO%d, TX=IO%d, %lu bodow\n",
                                      CONFIGS[cfgIdx].rx, CONFIGS[cfgIdx].tx, (unsigned long)CONFIGS[cfgIdx].baud);
                    }
                }
                len = 0;
            }
        } else if (len < sizeof(buf) - 1) {
            buf[len++] = c;
        } else {
            len = 0; // Za długa linia - śmieci (zła prędkość transmisji)
        }
    }

    logFixChanges(s);

    unsigned long now = millis();
    if (locked) {
        if (now - lastGoodMs > LOST_MS) {
            locked = false;
            s.gpsStatus = 0; s.gpsSats = 0;
            Serial.println("[GPS] Utracono polaczenie z modulem - szukam ponownie");
            applyConfig(cfgIdx);
        } else if (s.gpsStatus == 2 && now - lastFixMs > FIX_LOST_MS) {
            s.gpsStatus = 1; // Moduł nadaje, ale bez pozycji (tunel, garaż)
        } else if (s.gpsStatus == 0) {
            s.gpsStatus = 1;
        }
    } else if (now - cfgSinceMs > PROBE_MS) {
        applyConfig((cfgIdx + 1) % CONFIG_COUNT); // Następna kombinacja pinów / prędkości
    }
}

// Log: złapanie i utrata pozycji, a w trakcie szukania co 30 s ile satelitów widać
void GpsService::logFixChanges(MotoState& s) {
    unsigned long now = millis();
    bool fix = s.gpsStatus == 2;
    if (fix && !hadFix) {
        hadFix = true;
        Serial.printf("[GPS] Pozycja ustalona po %lu s: %d sat. (widocznych %d), HDOP %.1f, %.6f, %.6f, wys. %.0f m\n",
                      (now - searchStartMs) / 1000, s.gpsSats, satsInView(), s.gpsHdop, s.gpsLat, s.gpsLon, s.gpsAltM);
    } else if (!fix && hadFix) {
        hadFix = false;
        searchStartMs = now;
        lastSearchLogMs = now;
        Serial.println("[GPS] Utracono pozycje - szukam satelitow");
    } else if (!fix && locked && now - lastSearchLogMs >= 30000) {
        lastSearchLogMs = now;
        Serial.printf("[GPS] Szukam pozycji od %lu s, widocznych satelitow: %d (GPS %d, BeiDou %d, GLONASS %d, Galileo %d)\n",
                      (now - searchStartMs) / 1000, satsInView(), inView[0], inView[1], inView[2], inView[3]);
    }
}

// --- Parser NMEA ---------------------------------------------------------

static int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// ddmm.mmmm + półkula -> stopnie dziesiętne
static double nmeaCoord(const char* v, const char* hemi) {
    if (!v[0]) return 0.0;
    double raw = atof(v);
    int deg = (int)(raw / 100.0);
    double d = deg + (raw - deg * 100.0) / 60.0;
    if (hemi[0] == 'S' || hemi[0] == 'W') d = -d;
    return d;
}

bool GpsService::feedLine(const char* line, MotoState& s) {
    if (line[0] != '$') return false;
    const char* star = strchr(line, '*');
    if (!star || hexVal(star[1]) < 0 || hexVal(star[2]) < 0) return false;
    uint8_t sum = 0;
    for (const char* p = line + 1; p < star; p++) sum ^= (uint8_t)*p;
    if (sum != (uint8_t)(hexVal(star[1]) * 16 + hexVal(star[2]))) return false;

    // Kopia do podziału na pola (przecinki -> \0)
    char tmp[100];
    size_t n = star - line;
    if (n >= sizeof(tmp)) return false;
    memcpy(tmp, line, n);
    tmp[n] = 0;
    char* f[24];
    int cnt = 0;
    f[cnt++] = tmp;
    for (char* p = tmp; *p && cnt < 24; p++) {
        if (*p == ',') { *p = 0; f[cnt++] = p + 1; }
    }
    // Typ zdania bez prefiksu nadawcy: $GPRMC / $GNRMC / $BDRMC ...
    if (strlen(f[0]) < 6) return true;
    const char* type = f[0] + 3;
    if (strcmp(type, "RMC") == 0) handleRmc(f, cnt, s);
    else if (strcmp(type, "GGA") == 0) handleGga(f, cnt, s);
    else if (strcmp(type, "GSV") == 0) handleGsv(f[0] + 1, f, cnt);
    return true;
}

void GpsService::handleRmc(char** f, int n, MotoState& s) {
    if (n < 10) return;
    bool valid = f[2][0] == 'A';

    // Czas i data: hhmmss(.ss) i ddmmyy
    if (strlen(f[1]) >= 6 && strlen(f[9]) == 6) {
        int hh = (f[1][0]-'0')*10 + (f[1][1]-'0'), mi = (f[1][2]-'0')*10 + (f[1][3]-'0'), ss = (f[1][4]-'0')*10 + (f[1][5]-'0');
        int dd = (f[9][0]-'0')*10 + (f[9][1]-'0'), mo = (f[9][2]-'0')*10 + (f[9][3]-'0'), yy = (f[9][4]-'0')*10 + (f[9][5]-'0');
        // Przed ustaleniem pozycji niektóre moduły podają datę domyślną (np. 2080 r.) - taką pomijamy
        if (yy >= 24 && yy <= 70 && mo >= 1 && mo <= 12 && dd >= 1 && dd <= 31) {
            syncClock(utcToEpoch(2000 + yy, mo, dd, hh, mi, ss), valid);
        }
    }
    if (!valid) return;

    s.gpsLat      = nmeaCoord(f[3], f[4]);
    s.gpsLon      = nmeaCoord(f[5], f[6]);
    s.gpsSpeedKmh = f[7][0] ? (float)(atof(f[7]) * 1.852) : 0.0f; // węzły -> km/h
    if (f[8][0]) s.gpsCourse = (float)atof(f[8]);
    s.gpsStatus   = 2;
    s.gpsFixCount++;
    lastFixMs = millis();
}

// $xxGSV,liczba_zdan,nr_zdania,satelity_widoczne,... - liczymy osobno dla każdego systemu
void GpsService::handleGsv(const char* talker, char** f, int n) {
    if (n < 4) return;
    int idx = (talker[0] == 'G' && talker[1] == 'P') ? 0 :
              (talker[0] == 'B' && talker[1] == 'D') || (talker[0] == 'G' && talker[1] == 'B') ? 1 :
              (talker[0] == 'G' && talker[1] == 'L') ? 2 :
              (talker[0] == 'G' && talker[1] == 'A') ? 3 : -1;
    if (idx >= 0) inView[idx] = (uint8_t)atoi(f[3]);
}

void GpsService::handleGga(char** f, int n, MotoState& s) {
    if (n < 10) return;
    s.gpsSats = (uint8_t)atoi(f[7]);
    if (atoi(f[6]) == 0) return; // Brak pozycji
    if (f[8][0]) s.gpsHdop = (float)atof(f[8]);
    if (f[9][0]) s.gpsAltM = (float)atof(f[9]);
}

void GpsService::syncClock(time_t utc, bool fixValid) {
    time_t now = time(nullptr);
    long diff = (long)(now - utc);
    if (diff < 0) diff = -diff;
    // Ustawiamy przy pierwszym czasie, a potem tylko gdy zegar uciekł o >2 s (najwyżej raz na minutę)
    if (!clockSynced || (diff > 2 && fixValid && millis() - lastSyncMs > 60000UL)) {
        struct timeval tv = { utc, 0 };
        settimeofday(&tv, nullptr);
        clockSynced = true;
        lastSyncMs = millis();
        struct tm lt;
        localtime_r(&utc, &lt);
        Serial.printf("[GPS] Zegar ustawiony z GPS: %04d-%02d-%02d %02d:%02d:%02d\n",
                      lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
    }
}

// Liczba dni od 1970-01-01 (algorytm "days from civil", H. Hinnant) - bez zależności od strefy czasowej
time_t GpsService::utcToEpoch(int y, int mon, int d, int h, int mi, int s) {
    y -= mon <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (mon + (mon > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    long days = era * 146097 + (long)doe - 719468;
    return (time_t)(days * 86400L + h * 3600L + mi * 60L + s);
}
