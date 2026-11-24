#include "CanReader.h"
#include "Config.h"
#include "driver/twai.h"
#include <string.h>

// Kolejno pytamy o te parametry (OBD-II, tryb 01). 0x01 = liczba błędów (rzadziej, co ok. 5 s).
static const uint8_t PIDS[] = { 0x0C /*obroty*/, 0x0D /*prędkość*/, 0x0B /*MAP*/, 0x0C, 0x0D, 0x0F /*temp. powietrza*/,
                                0x05 /*temp. płynu*/, 0x42 /*napięcie*/ };
static const uint8_t PID_COUNT = sizeof(PIDS) / sizeof(PIDS[0]);

struct Combo { uint32_t kbps; bool swapped; };
static const Combo COMBOS[] = { {500, false}, {250, false}, {500, true}, {250, true} };
static const uint8_t COMBO_COUNT = sizeof(COMBOS) / sizeof(COMBOS[0]);

static const unsigned long PROBE_WAIT_MS  = 300;   // Czekanie na odpowiedź przy wykrywaniu
static const unsigned long PROBE_PAUSE_MS = 3000;  // Przerwa po nieudanej rundzie wykrywania
static const unsigned long REPLY_WAIT_MS  = 120;   // Czekanie na odpowiedź na jedno zapytanie
static const unsigned long LOST_MS        = 4000;  // Bez odpowiedzi przez tyle ms = sterownik wyłączony

void CanReader::init() {
    if (!Config::CAN_ENABLED) {
        mode = SIMULATION;
        Serial.println("[CAN] Brak modulu CAN w konfiguracji - symulacja silnika");
        return;
    }
    mode = PROBING;
    combo = 0;
    nextProbeMs = 0;
    Serial.println("[CAN] Szukam sterownika OBD-II na zlaczu IO15/IO16 (CTX=IO15, CRX=IO16)...");
}

bool CanReader::startDriver(uint8_t i) {
    stopDriver();
    gpio_num_t tx = (gpio_num_t)(COMBOS[i].swapped ? Config::PIN_CAN_B : Config::PIN_CAN_A);
    gpio_num_t rx = (gpio_num_t)(COMBOS[i].swapped ? Config::PIN_CAN_A : Config::PIN_CAN_B);
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(tx, rx, TWAI_MODE_NORMAL);
    g.rx_queue_len = 32;
    twai_timing_config_t t = COMBOS[i].kbps == 500 ? (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS()
                                                   : (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS();
    // Tylko odpowiedzi diagnostyczne 0x7E8..0x7EF (11-bit) - reszta ruchu na szynie nas nie obchodzi
    twai_filter_config_t f;
    f.acceptance_code = (0x7E8u << 21);
    f.acceptance_mask = ~(0x7F8u << 21);
    f.single_filter   = true;
    if (twai_driver_install(&g, &t, &f) != ESP_OK) return false;
    if (twai_start() != ESP_OK) { twai_driver_uninstall(); return false; }
    driverOn = true;
    return true;
}

void CanReader::stopDriver() {
    if (!driverOn) return;
    twai_stop();
    twai_driver_uninstall();
    driverOn = false;
}

// Zapytanie OBD-II w ramce ISO-TP "single frame" na adres ogólny 0x7DF
bool CanReader::sendPid(uint8_t service, uint8_t pid) {
    twai_message_t m = {};
    m.identifier = 0x7DF;
    m.data_length_code = 8;
    uint8_t d[8] = { (uint8_t)(service == 0x04 ? 1 : 2), service, pid, 0x55, 0x55, 0x55, 0x55, 0x55 };
    memcpy(m.data, d, 8);
    return twai_transmit(&m, 0) == ESP_OK;
}

// Odbiera jedną odpowiedź i wpisuje wartość do stanu. Zwraca true, gdy przyszła odpowiedź trybu 01.
bool CanReader::readReply(MotoState& s, uint8_t& pidOut) {
    twai_message_t m;
    while (twai_receive(&m, 0) == ESP_OK) {
        if (m.extd || m.identifier < 0x7E8 || m.identifier > 0x7EF || m.data_length_code < 3) continue;
        const uint8_t* d = m.data;
        if (d[1] == 0x44) { Serial.println("[CAN] Sterownik potwierdzil skasowanie bledow"); continue; }
        if (d[1] != 0x41) continue;
        uint8_t A = d[3], B = d[4];
        switch (d[2]) {
            case 0x0C: s.rpm         = ((A << 8) | B) / 4; break;
            case 0x0D: s.speedKmh    = A; break;
            case 0x05: s.engineTempC = (int)A - 40; break;
            case 0x0B: s.mapKpa      = A; break;
            case 0x0F: s.iatTempC    = (int)A - 40; break;
            case 0x42: s.voltageV    = ((A << 8) | B) / 1000.0f; break;
            case 0x01: s.dtcCount    = A & 0x7F; break;
            default: break;
        }
        pidOut = d[2];
        return true;
    }
    return false;
}

void CanReader::update(MotoState& s) {
    unsigned long now = millis();

    if (mode == SIMULATION) { simulate(s); return; }

    if (mode == PROBING) {
        if (!driverOn) {
            if (now < nextProbeMs) return;
            if (!startDriver(combo)) { combo = (combo + 1) % COMBO_COUNT; return; }
            sendPid(0x01, 0x0C);
            probeSentMs = now;
            return;
        }
        uint8_t pid;
        if (readReply(s, pid)) {
            mode = CONNECTED;
            lastReplyMs = now;
            waiting = false;
            pidIdx = 0;
            memset(misses, 0, sizeof(misses));
            memset(unsupported, 0, sizeof(unsupported));
            Serial.printf("[CAN] Polaczono ze sterownikiem: %lu kb/s, CTX=IO%d, CRX=IO%d\n", (unsigned long)COMBOS[combo].kbps,
                          COMBOS[combo].swapped ? Config::PIN_CAN_B : Config::PIN_CAN_A,
                          COMBOS[combo].swapped ? Config::PIN_CAN_A : Config::PIN_CAN_B);
            return;
        }
        if (now - probeSentMs > PROBE_WAIT_MS) {
            // Diagnostyka: liczniki błędów mówią, czy ktoś na szynie potwierdza naszą ramkę
            twai_status_info_t st;
            if ((probeRounds < 2 || now - lastDiagMs > 30000) && twai_get_status_info(&st) == ESP_OK) {
                if (combo == COMBO_COUNT - 1) lastDiagMs = now;
                const char* why = st.bus_error_count > 0 ? "bledy na szynie (zla predkosc lub zamienione CANH/CANL)"
                                : st.tx_error_counter >= 128 ? "nikt nie potwierdza ramki (zaplon? kable CANH/CANL? masa?)"
                                : st.msgs_to_tx > 0 ? "ramka nie wyszla z modulu (CTX/CRX? zasilanie 3V3?)"
                                : "ramka wyslana i potwierdzona, ale brak odpowiedzi OBD";
                Serial.printf("[CAN] Proba %lu kb/s, CTX=IO%d: bledy TX %lu, RX %lu, szyny %lu, do wyslania %lu -> %s\n",
                              (unsigned long)COMBOS[combo].kbps,
                              COMBOS[combo].swapped ? Config::PIN_CAN_B : Config::PIN_CAN_A,
                              (unsigned long)st.tx_error_counter, (unsigned long)st.rx_error_counter,
                              (unsigned long)st.bus_error_count, (unsigned long)st.msgs_to_tx, why);
            }
            stopDriver();
            combo = (combo + 1) % COMBO_COUNT;
            if (combo == 0) {
                probeRounds++;
                nextProbeMs = now + PROBE_PAUSE_MS; // Cała runda bez odpowiedzi - zapłon wyłączony?
                if (now - lastLogMs > 20000) {
                    lastLogMs = now;
                    Serial.println("[CAN] Brak odpowiedzi sterownika (zaplon wlaczony? kable CANH/CANL?) - probuje dalej");
                }
            }
        }
        return;
    }

    // --- CONNECTED: jedno zapytanie naraz, kolejno po liście ---
    uint8_t pid;
    while (readReply(s, pid)) {
        lastReplyMs = now;
        for (uint8_t i = 0; i < PID_COUNT; i++) if (PIDS[i] == pid) misses[i] = 0;
        waiting = false;
    }
    if (waiting && now - reqSentMs > REPLY_WAIT_MS) {
        waiting = false;
        if (++misses[pidIdx] >= 5 && !unsupported[pidIdx]) {
            // Parametr nieobsługiwany przez ten sterownik - pomijamy go (np. napięcie w starszych autach)
            for (uint8_t i = 0; i < PID_COUNT; i++) if (PIDS[i] == PIDS[pidIdx]) unsupported[i] = true;
            Serial.printf("[CAN] Sterownik nie podaje parametru 0x%02X - pomijam\n", PIDS[pidIdx]);
        }
    }
    if (!waiting) {
        // Co ok. 5 s zamiast kolejnego parametru pytamy o liczbę błędów (PID 0x01)
        static unsigned long lastDtcMs = 0;
        bool askDtc = now - lastDtcMs > 5000;
        if (!askDtc) {
            for (uint8_t k = 0; k < PID_COUNT; k++) {
                pidIdx = (pidIdx + 1) % PID_COUNT;
                if (!unsupported[pidIdx]) break;
            }
        }
        if (sendPid(0x01, askDtc ? 0x01 : PIDS[pidIdx])) {
            if (askDtc) lastDtcMs = now;
            waiting = !askDtc;
            reqSentMs = now;
        }
    }

    if (now - lastReplyMs > LOST_MS) {
        Serial.println("[CAN] Sterownik przestal odpowiadac - szukam ponownie");
        stopDriver();
        mode = PROBING;
        nextProbeMs = now + 1000;
        s.rpm = 0; s.speedKmh = 0;
        return;
    }

    if (now - lastLogMs > 10000) {
        lastLogMs = now;
        Serial.printf("[CAN] obroty %d, predkosc %d km/h, plyn %d C, MAP %d kPa, powietrze %d C, napiecie %.1f V, bledy %d\n",
                      s.rpm, s.speedKmh, s.engineTempC, s.mapKpa, s.iatTempC, s.voltageV, s.dtcCount);
    }
    updateStats(s);
}

void CanReader::simulate(MotoState& state) {
    // Ograniczamy częstotliwość do 5 razy na sekundę (co 200 ms)
    if (millis() - lastPollMs < 200) return;
    lastPollMs = millis();

    state.rpm += random(-300, 600);
    if (state.rpm > 10200) state.rpm = 4500;
    if (state.rpm < 1500)  state.rpm = 3500;

    state.speedKmh    = (state.rpm / 1000) * 14;
    state.mapKpa      = map(state.rpm, 1500, 10200, 30, 95);
    state.engineTempC = constrain(state.engineTempC + random(-1, 2), 84, 105);
    state.voltageV    = 14.2f;
    updateStats(state);
}

// Wartości szczytowe dla ekranu Statystyk
void CanReader::updateStats(MotoState& state) {
    if (state.speedKmh > state.maxSpeedKmh)       state.maxSpeedKmh = state.speedKmh;
    if (state.rpm > state.maxRpm)                 state.maxRpm = state.rpm;
    if (state.engineTempC > state.maxEngineTempC) state.maxEngineTempC = state.engineTempC;
    if (state.voltageV > 5.0f && state.voltageV < state.minBootVoltageV) state.minBootVoltageV = state.voltageV;
}

void CanReader::clearDtcCodes(MotoState& state) {
    if (mode == CONNECTED) {
        sendPid(0x04, 0x00); // OBD-II tryb 04 - jak "kasuj błędy" w czytniku OBD2
        Serial.println("[CAN] Wyslano kasowanie bledow do sterownika");
    }
    state.dtcCount = 0;
}

void CanReader::resetStats(MotoState& state) {
    state.maxRpm          = state.rpm;
    state.maxSpeedKmh     = state.speedKmh;
    state.maxEngineTempC  = state.engineTempC;
    state.minBootVoltageV = state.voltageV;
    Serial.println("[CanReader] Zresetowano statystyki Max/Min");
}
