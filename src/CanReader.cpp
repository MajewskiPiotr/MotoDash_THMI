#include "CanReader.h"
#include <Arduino.h>

void CanReader::init() {
    // Docelowo: inicjalizacja transceivera MCP2515 lub TWAI na 500 kbps
    Serial.println("[CanReader] Inicjalizacja szyny CAN");
}

void CanReader::update(MotoState& state) {
    // Ograniczamy czÄ™stotliwoĹ›Ä‡ zapytaĹ„ do 5 razy na sekundÄ™ (co 200 ms)
    if (millis() - lastPollMs < 200) return;
    lastPollMs = millis();

    // Symulacja pracy silnika na potrzeby testĂłw w symulatorze Wokwi
    state.rpm += random(-300, 600);
    if (state.rpm > 10200) state.rpm = 4500;
    if (state.rpm < 1500)  state.rpm = 3500;

    state.speedKmh    = (state.rpm / 1000) * 14;
    state.mapKpa      = map(state.rpm, 1500, 10200, 30, 95);
    state.engineTempC = constrain(state.engineTempC + random(-1, 2), 84, 105);
    state.voltageV    = 14.2f;

// Aktualizacja prędkości maksymalnej
if (state.speedKmh > state.maxSpeedKmh) {
    state.maxSpeedKmh = state.speedKmh;
}
    // Ĺšledzenie wartoĹ›ci szczytowych dla Ekranu 2 (Statystyki Motocykla)
    if (state.rpm > state.maxRpm) {
        state.maxRpm = state.rpm;
    }
    if (state.engineTempC > state.maxEngineTempC) {
        state.maxEngineTempC = state.engineTempC;
    }
    if (state.voltageV > 5.0f && state.voltageV < state.minBootVoltageV) {
        state.minBootVoltageV = state.voltageV;
    }
}

void CanReader::clearDtcCodes(MotoState& state) {
    state.dtcCount = 0;
    Serial.println("[CanReader] Skasowano bledy ECU");
}

void CanReader::resetStats(MotoState& state) {
    state.maxRpm          = state.rpm;
    state.maxSpeedKmh     = state.speedKmh; // Resetuje prędkość maksymalną do bieżącej
    state.maxEngineTempC  = state.engineTempC;
    state.minBootVoltageV = state.voltageV;
    Serial.println("[CanReader] Zresetowano statystyki Max/Min");
}
