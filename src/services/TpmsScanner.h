#pragma once
#include <Arduino.h>
#include "MotoState.h"

// Odczyt czujników ciśnienia w oponach (TPMS) przez Bluetooth LE.
//
// Czujniki nie wymagają parowania - co kilka sekund nadają ogłoszenie z danymi, a MotoDash
// nasłuchuje w tle (pasywne skanowanie). Obsługiwany format tanich nakrętek na wentyl
// (aplikacje typu "TPMS"/"TPMS II"): dane producenta 18 bajtów:
//   [0-1] 00 01 | [2-7] adres czujnika (pierwszy bajt 0x80 + numer-1) |
//   [8-11] ciśnienie [Pa] LE | [12-15] temperatura [°C x100] LE | [16] bateria [%] | [17] alarm
// Inny format? Włącz Config::TPMS_DEBUG_SCAN i wyślij log - dopiszemy dekoder.
class TpmsScanner {
public:
    void init();
    void update(MotoState& state); // Wywoływać w loop(): przenosi odczyty do MotoState, pilnuje skanowania

    // Wywoływane z zadania Bluetooth (inny wątek!) - tylko zapis do bufora pod blokadą
    void onAdvertisement(const String& addr, const String& name, int rssi, const uint8_t* mfg, size_t mfgLen,
                         const String& serviceInfo);

private:
    struct Reading {
        bool          fresh   = false; // Nowy odczyt czeka na przeniesienie do MotoState
        unsigned long atMs    = 0;     // Kiedy odebrano (0 = nigdy)
        float         bar     = 0;
        int           tempC   = 0;
        int           battPct = -1;
    };
    Reading front, rear;
    portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
    unsigned long lastScanCheckMs = 0;
    // Diagnostyka (tryb rozpoznawania): ile ogłoszeń BLE odebrano i jaki najsilniejszy sygnał
    volatile uint32_t advCount = 0;
    volatile int      advBestRssi = -127;
    unsigned long     lastStatsMs = 0;
    bool started = false;

    int  wheelFor(const String& addr, uint8_t sensorNo) const; // 0 = przód, 1 = tył, -1 = nie nasz
    void apply(Reading& r, bool& ok, float& bar, int& tempC, int& batt);
};
