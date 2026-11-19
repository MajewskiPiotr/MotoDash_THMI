#pragma once
#include "MotoState.h"

// Serwis komunikujÄ…cy siÄ™ z szynÄ… CAN / OBD-II motocykla.
class CanReader {
public:
    void init();                           // Konfiguruje transceiver CAN
    void update(MotoState& state);         // Pobiera parametry z ECU i aktualizuje wartoĹ›ci Max/Min
    void clearDtcCodes(MotoState& state);  // WysyĹ‚a ĹĽÄ…danie skasowania bĹ‚Ä™dĂłw sterownika silnika
    void resetStats(MotoState& state);     // Zeruje statystyki szczytowe (Max RPM, Max Temp, Min V)

private:
    unsigned long lastPollMs = 0;          // Czas ostatniego odpytania ECU [ms]
};
