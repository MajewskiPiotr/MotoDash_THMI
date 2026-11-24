#pragma once
#include <Arduino.h>
#include "MotoState.h"

// Odczyt danych silnika przez CAN / OBD-II (standardowe zapytania tylko do odczytu, jak tani czytnik OBD2).
// Moduł: transceiver SN65HVD230 (3V3, GND, CTX, CRX) + wbudowany w ESP32 kontroler CAN (TWAI).
// - sam wykrywa prędkość szyny (500 / 250 kb/s) i kierunek pinów,
// - co ok. 0,1 s pyta o kolejny parametr: obroty, prędkość, temp. płynu, MAP, temp. powietrza, napięcie, błędy,
// - gdy sterownik przestanie odpowiadać (zapłon wyłączony), co kilka sekund próbuje połączyć się od nowa.
// Moduł w złączu IO15/IO16. Gdy Config::CAN_ENABLED == false, działa symulacja silnika.
class CanReader {
public:
    void init();
    void update(MotoState& state);
    void clearDtcCodes(MotoState& state);  // OBD tryb 04: kasuje zapisane błędy w sterowniku
    void resetStats(MotoState& state);

private:
    enum Mode : uint8_t { SIMULATION, PROBING, CONNECTED };
    Mode mode = SIMULATION;

    // Wykrywanie: kombinacja = prędkość x kierunek pinów
    uint8_t       combo        = 0;
    bool          driverOn     = false;
    unsigned long probeSentMs  = 0;
    unsigned long nextProbeMs  = 0;

    // Odpytywanie
    uint8_t       pidIdx       = 0;
    bool          waiting      = false;
    unsigned long reqSentMs    = 0;
    unsigned long lastReplyMs  = 0;
    unsigned long lastLogMs    = 0;
    uint8_t       misses[8]    = {0};   // Kolejne braki odpowiedzi na dany parametr
    bool          unsupported[8] = {false};

    unsigned long lastPollMs   = 0;     // Symulacja

    bool startDriver(uint8_t comboIdx);
    void stopDriver();
    bool sendPid(uint8_t service, uint8_t pid);
    bool readReply(MotoState& s, uint8_t& pidOut);
    void updateStats(MotoState& s);
    void simulate(MotoState& s);
};
