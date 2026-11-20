#include "TpmsScanner.h"
#include <Arduino.h>

void TpmsScanner::init() {
    Serial.println("[TpmsScanner] Inicjalizacja skanera BLE TPMS");
}

void TpmsScanner::update(MotoState& state) {
    if (millis() - lastScanMs < 3000) return;
    lastScanMs = millis();

    // Tymczasowe dane testowe (docelowo dekodowanie ramek BLE z opon)
    state.frontBar   = 2.3f;
    state.frontTempC = 25;
    state.rearBar    = 2.6f;
    state.rearTempC  = 28;
}
