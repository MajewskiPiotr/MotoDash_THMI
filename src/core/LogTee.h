#pragma once
#include <Arduino.h>

// Log na USB i jednocześnie do pliku /log.txt na karcie microSD (do czytania bez laptopa:
// wyjmij kartę albo pobierz plik ze strony Wi-Fi MotoDash, przycisk "Log").
// Cały kod projektu pisze przez Serial.print/println/printf - makro poniżej kieruje to tutaj.
class LogTee : public Print {
public:
    void   begin(unsigned long baud);
    size_t write(uint8_t c) override;
    size_t write(const uint8_t* buf, size_t len) override;
    using Print::write;

    void attachSd(bool sdOk);   // Wywołać po zamontowaniu karty (zapisuje też logi sprzed montażu)
    void loop();                // Zapis bufora na kartę (co ~2 s albo gdy bufor się zapełni)
    bool hasSd() const { return sd; }

    static const char* PATH;    // "/log.txt"

private:
    static constexpr size_t BUF = 16384; // Zapas na logi sprzed zamontowania karty i między zapisami
    char     buf[BUF];
    size_t   len = 0;
    bool     sd = false;
    unsigned long lastFlushMs = 0;
    void flushToSd(bool force);
};

extern LogTee LogSerial;

// Wszystkie Serial.* w projekcie idą przez LogTee (Serial1 itp. bez zmian)
// (W rdzeniu ESP32 z USB CDC "Serial" bywa już makrem, np. HWCDCSerial - nadpisujemy je bez ostrzeżenia)
#ifndef LOGTEE_IMPL
#ifdef Serial
#undef Serial
#endif
#define Serial LogSerial
#endif
