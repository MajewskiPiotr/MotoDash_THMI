#define LOGTEE_IMPL
#include "LogTee.h"
#include <SD_MMC.h>
#include <string.h>

LogTee LogSerial;
const char* LogTee::PATH = "/log.txt";

static portMUX_TYPE logLock = portMUX_INITIALIZER_UNLOCKED; // Log piszą też wątki Bluetooth
static const size_t MAX_FILE = 2UL * 1024 * 1024;           // Powyżej 2 MB: /log.txt -> /log_stary.txt

void LogTee::begin(unsigned long baud) { Serial.begin(baud); }

size_t LogTee::write(uint8_t c) { return write(&c, 1); }

size_t LogTee::write(const uint8_t* data, size_t n) {
    Serial.write(data, n); // USB jak dotąd
    portENTER_CRITICAL(&logLock);
    size_t room = BUF - len;
    size_t k = n < room ? n : room; // Przy przepełnieniu (karta wolna/brak) gubimy nadmiar, nie blokujemy
    memcpy(buf + len, data, k);
    len += k;
    portEXIT_CRITICAL(&logLock);
    return n;
}

void LogTee::attachSd(bool sdOk) {
    sd = sdOk;
    if (!sd) return;
    // Rotacja zbyt dużego pliku
    if (SD_MMC.exists(PATH)) {
        fs::File f = SD_MMC.open(PATH, FILE_READ);
        size_t size = f ? f.size() : 0;
        if (f) f.close();
        if (size > MAX_FILE) {
            SD_MMC.remove("/log_stary.txt");
            SD_MMC.rename(PATH, "/log_stary.txt");
        }
    }
    fs::File f = SD_MMC.open(PATH, FILE_APPEND);
    if (f) { f.print("\n===== START MotoDash =====\n"); f.close(); }
    flushToSd(true); // Logi z początku uruchomienia (sprzed zamontowania karty)
}

void LogTee::loop() { flushToSd(false); }

void LogTee::flushToSd(bool force) {
    if (!sd || len == 0) return;
    unsigned long now = millis();
    if (!force && len < BUF / 2 && now - lastFlushMs < 2000) return;
    lastFlushMs = now;

    static char out[BUF];
    portENTER_CRITICAL(&logLock);
    size_t n = len;
    memcpy(out, buf, n);
    len = 0;
    portEXIT_CRITICAL(&logLock);

    // Otwórz-dopisz-zamknij: nagłe wyłączenie zasilania nie psuje pliku
    fs::File f = SD_MMC.open(PATH, FILE_APPEND);
    if (f) { f.write((const uint8_t*)out, n); f.close(); }
}
