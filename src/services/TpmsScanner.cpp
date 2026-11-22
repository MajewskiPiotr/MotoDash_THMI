#include "TpmsScanner.h"
#include "Config.h"
#include <NimBLEDevice.h>

static TpmsScanner* s_instance = nullptr;

// Czy tekst (wielkie litery, bez separatorów) zawiera identyfikator z Config - w zwykłej lub odwróconej kolejności bajtów
static bool containsDebugId(const String& hexNoSep) {
    String id = String(Config::TPMS_DEBUG_ID);
    id.toUpperCase();
    if (id.length() < 4) return false;
    if (hexNoSep.indexOf(id) >= 0) return true;
    String rev;
    for (int i = (int)id.length() - 2; i >= 0; i -= 2) rev += id.substring(i, i + 2); // np. C225B17B -> 7BB125C2
    return hexNoSep.indexOf(rev) >= 0;
}

// Callback NimBLE - wywoływany dla każdego odebranego ogłoszenia (także powtórzeń)
class TpmsAdvCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* dev) override {
        if (!s_instance) return;
        std::string mfg = dev->haveManufacturerData() ? dev->getManufacturerData() : std::string();
        // Usługi i dane usług - tylko do trybu rozpoznawania (niektóre czujniki nadają dane tutaj)
        String svc;
        if (Config::TPMS_DEBUG_SCAN) {
            for (int i = 0; i < (int)dev->getServiceDataCount(); i++) {
                std::string d = dev->getServiceData(i);
                svc += " usluga " + String(dev->getServiceDataUUID(i).toString().c_str()) + ":";
                for (unsigned char c : d) { char b[4]; snprintf(b, sizeof(b), " %02X", c); svc += b; }
            }
            for (int i = 0; i < (int)dev->getServiceUUIDCount(); i++)
                svc += " uuid " + String(dev->getServiceUUID(i).toString().c_str());
        }
        s_instance->onAdvertisement(String(dev->getAddress().toString().c_str()),
                                    dev->haveName() ? String(dev->getName().c_str()) : String(""),
                                    dev->getRSSI(), (const uint8_t*)mfg.data(), mfg.size(), svc);
    }
};

void TpmsScanner::init() {
    s_instance = this;
    if (Config::TPMS_FRONT_NO == 0 && Config::TPMS_REAR_NO == 0 &&
        !Config::TPMS_FRONT_MAC[0] && !Config::TPMS_REAR_MAC[0] && !Config::TPMS_DEBUG_SCAN) {
        Serial.println("[TPMS] Brak skonfigurowanych czujnikow - Bluetooth wylaczony");
        return;
    }
    NimBLEDevice::init("");
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new TpmsAdvCallbacks(), true); // true = także powtórzenia ogłoszeń
    scan->setActiveScan(false); // Pasywnie: tylko słuchamy, nic nie wysyłamy
    scan->setInterval(160);     // 100 ms
    scan->setWindow(48);        // 30 ms - zostawia czas radia dla Wi-Fi
    scan->setMaxResults(0);     // Nie przechowujemy listy urządzeń (oszczędza RAM)
    scan->start(0, nullptr, false);
    started = true;
    Serial.println("[TPMS] Nasluchuje czujnikow opon (BLE). Przod: nr " + String(Config::TPMS_FRONT_NO) +
                   ", tyl: nr " + String(Config::TPMS_REAR_NO));
}

// Który czujnik to które koło: adres z Config ma pierwszeństwo, potem numer czujnika (1. bajt adresu w danych)
int TpmsScanner::wheelFor(const String& addr, uint8_t sensorNo) const {
    if (Config::TPMS_FRONT_MAC[0] && addr.equalsIgnoreCase(Config::TPMS_FRONT_MAC)) return 0;
    if (Config::TPMS_REAR_MAC[0]  && addr.equalsIgnoreCase(Config::TPMS_REAR_MAC))  return 1;
    if (!Config::TPMS_FRONT_MAC[0] && sensorNo && sensorNo == Config::TPMS_FRONT_NO) return 0;
    if (!Config::TPMS_REAR_MAC[0]  && sensorNo && sensorNo == Config::TPMS_REAR_NO)  return 1;
    return -1;
}

void TpmsScanner::onAdvertisement(const String& addr, const String& name, int rssi, const uint8_t* mfg, size_t len,
                                  const String& serviceInfo) {
    advCount++;
    if (rssi > advBestRssi) advBestRssi = rssi;

    // --- Szukanie czujnika po identyfikatorze z obudowy (adres + wszystkie dane, bez filtra RSSI) ---
    if (Config::TPMS_DEBUG_SCAN && Config::TPMS_DEBUG_ID[0]) {
        String all = addr + " " + serviceInfo + " ";
        String hexSp;
        for (size_t i = 0; i < len; i++) { char b[4]; snprintf(b, sizeof(b), "%02X ", mfg[i]); hexSp += b; }
        all += hexSp;
        all.toUpperCase();
        String packed;
        for (unsigned int i = 0; i < all.length(); i++) { char c = all[i]; if (isxdigit((unsigned char)c)) packed += c; }
        if (containsDebugId(packed)) {
            static unsigned long lastIdLog = 0;
            if (millis() - lastIdLog > 3000) {
                lastIdLog = millis();
                Serial.printf("[TPMS-ID] ZNALEZIONO: %s RSSI %d nazwa \"%s\" dane(%u): %s%s\n",
                              addr.c_str(), rssi, name.c_str(), (unsigned)len, hexSp.c_str(), serviceInfo.c_str());
            }
        }
    }
    // --- Format nakrętek TPMS (18 bajtów danych producenta, firma 0x0001, adres zaczyna się od 0x80..0x83) ---
    bool known = len == 18 && mfg[0] == 0x00 && mfg[1] == 0x01 && mfg[2] >= 0x80 && mfg[2] <= 0x83;
    if (known) {
        uint8_t  sensorNo = mfg[2] - 0x80 + 1;
        uint32_t pa  = (uint32_t)mfg[8]  | ((uint32_t)mfg[9]  << 8) | ((uint32_t)mfg[10] << 16) | ((uint32_t)mfg[11] << 24);
        int32_t  t100 = (int32_t)((uint32_t)mfg[12] | ((uint32_t)mfg[13] << 8) | ((uint32_t)mfg[14] << 16) | ((uint32_t)mfg[15] << 24));
        float    bar  = pa / 100000.0f;
        int      tC   = (int)roundf(t100 / 100.0f);
        int      batt = mfg[16];

        int wheel = wheelFor(addr, sensorNo);
        if (wheel >= 0) {
            Reading& r = wheel == 0 ? front : rear;
            portENTER_CRITICAL(&lock);
            r.fresh = true; r.atMs = millis(); r.bar = bar; r.tempC = tC; r.battPct = batt;
            portEXIT_CRITICAL(&lock);
        }
        // Log co 10 s na czujnik (czujnik nadaje często)
        static unsigned long lastLog[4] = {0, 0, 0, 0};
        unsigned long& ll = lastLog[sensorNo - 1];
        if (millis() - ll > 10000) {
            ll = millis();
            Serial.printf("[TPMS] Czujnik nr %u (%s): %.2f bar, %d C, bateria %d%%, RSSI %d -> %s\n",
                          sensorNo, addr.c_str(), bar, tC, batt, rssi,
                          wheel == 0 ? "PRZOD" : wheel == 1 ? "TYL" : "nieprzypisany (Config.h)");
        }
        return;
    }

    // --- Tryb rozpoznawania: surowe dane z bliskich urządzeń z danymi producenta ---
    // Pomijamy szum od komputerów i telefonów: Microsoft (0x0006) i Apple (0x004C)
    bool noise = len >= 2 && ((mfg[0] == 0x06 && mfg[1] == 0x00) || (mfg[0] == 0x4C && mfg[1] == 0x00));
    bool hasData = len >= 2 || serviceInfo.length() > 0 || name.length() > 0;
    if (Config::TPMS_DEBUG_SCAN && !noise && hasData && rssi >= Config::TPMS_DEBUG_MIN_RSSI) {
        static String lastAddr[8];
        static unsigned long lastAt[8] = {0};
        int slot = -1, oldest = 0;
        for (int i = 0; i < 8; i++) {
            if (lastAddr[i] == addr) { slot = i; break; }
            if (lastAt[i] < lastAt[oldest]) oldest = i;
        }
        if (slot >= 0 && millis() - lastAt[slot] < 15000) return; // To samo urządzenie najwyżej co 15 s
        if (slot < 0) { slot = oldest; lastAddr[slot] = addr; }
        lastAt[slot] = millis();

        String hex;
        for (size_t i = 0; i < len; i++) { char b[4]; snprintf(b, sizeof(b), "%02X ", mfg[i]); hex += b; }
        Serial.printf("[TPMS-skan] %s RSSI %d nazwa \"%s\" dane(%u): %s%s\n",
                      addr.c_str(), rssi, name.c_str(), (unsigned)len, hex.c_str(), serviceInfo.c_str());
    }
}

void TpmsScanner::apply(Reading& r, bool& ok, float& bar, int& tempC, int& batt) {
    portENTER_CRITICAL(&lock);
    Reading copy = r;
    r.fresh = false;
    portEXIT_CRITICAL(&lock);

    if (copy.fresh) { bar = copy.bar; tempC = copy.tempC; batt = copy.battPct; }
    ok = copy.atMs != 0 && (millis() - copy.atMs) < Config::TPMS_STALE_MS;
}

void TpmsScanner::update(MotoState& state) {
    apply(front, state.frontTpmsOk, state.frontBar, state.frontTempC, state.frontBattPct);
    apply(rear,  state.rearTpmsOk,  state.rearBar,  state.rearTempC,  state.rearBattPct);

    // Diagnostyka co 30 s: potwierdza, że Bluetooth w ogóle odbiera ogłoszenia
    if (started && Config::TPMS_DEBUG_SCAN && millis() - lastStatsMs > 30000) {
        lastStatsMs = millis();
        Serial.printf("[TPMS] Diagnostyka: %u ogloszen BLE w 30 s, najsilniejszy sygnal %d dBm. Przod: %s, tyl: %s\n",
                      (unsigned)advCount, advBestRssi, state.frontTpmsOk ? "OK" : "brak", state.rearTpmsOk ? "OK" : "brak");
        advCount = 0;
        advBestRssi = -127;
    }

    // Skanowanie potrafi się zatrzymać (np. przy włączaniu Wi-Fi) - wznawiamy co 5 s
    if (started && millis() - lastScanCheckMs > 5000) {
        lastScanCheckMs = millis();
        NimBLEScan* scan = NimBLEDevice::getScan();
        if (!scan->isScanning()) scan->start(0, nullptr, false);
    }
}
