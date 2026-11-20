#pragma once
#include "MotoState.h"

// Serwis komunikujący się z szyną CAN / OBD-II motocykla.
class CanReader {
public:
    void init();                           // Konfiguruje transceiver CAN
    void update(MotoState& state);         // Pobiera parametry z ECU i aktualizuje wartości Max/Min
    void clearDtcCodes(MotoState& state);  // Wysyła żądanie skasowania błędów sterownika silnika
    void resetStats(MotoState& state);     // Zeruje statystyki szczytowe (Max RPM, Max Temp, Min V)

private:
    unsigned long lastPollMs = 0;          // Czas ostatniego odpytania ECU [ms]
};
