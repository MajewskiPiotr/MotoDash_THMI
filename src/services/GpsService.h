#pragma once
#include <Arduino.h>
#include "MotoState.h"

// Odbiornik GPS/GNSS (np. Seeed Grove GPS Air530Z) na złączu Grove IO17/IO18.
// - sam wykrywa, który pin nadaje, a który odbiera, oraz prędkość transmisji (9600/115200/38400/4800),
// - czyta zdania NMEA RMC (czas, data, pozycja, prędkość) i GGA (satelity, HDOP, wysokość),
// - ustawia zegar ESP32 (UTC; czas polski daje strefa TZ ustawiona przy starcie).
// Bez modułu nic się nie dzieje - gpsStatus zostaje 0 i funkcje GPS są ukryte.
class GpsService {
public:
    void init();
    void update(MotoState& state);

    // Przetwarza jedną linię NMEA (bez \r\n). Zwraca true, gdy suma kontrolna się zgadza.
    // Publiczne, żeby dało się testować parser bez modułu.
    bool feedLine(const char* line, MotoState& state);

    static time_t utcToEpoch(int y, int mon, int d, int h, int mi, int s); // Data UTC -> czas UNIX

private:
    struct PortCfg { uint8_t rx, tx; uint32_t baud; };
    static const PortCfg CONFIGS[];
    static const uint8_t CONFIG_COUNT;

    uint8_t       cfgIdx      = 0;
    unsigned long cfgSinceMs  = 0;
    bool          locked      = false; // Moduł znaleziony na bieżącej konfiguracji
    uint8_t       goodCount   = 0;     // Poprawne zdania na bieżącej konfiguracji
    unsigned long lastGoodMs  = 0;     // Ostatnie poprawne zdanie NMEA
    unsigned long lastFixMs   = 0;     // Ostatnie zdanie z ustaloną pozycją
    bool          clockSynced = false;
    unsigned long lastSyncMs  = 0;

    char    buf[100];
    uint8_t len = 0;

    void applyConfig(uint8_t i);
    void handleRmc(char** f, int n, MotoState& s);
    void handleGga(char** f, int n, MotoState& s);
    void syncClock(time_t utc, bool fixValid);
};
