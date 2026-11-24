#pragma once          // Zabezpiecza plik przed wielokrotnym dołączeniem podczas kompilacji
#include <Arduino.h>  // Podstawowe typy danych (uint8_t, String itp.)
#include "LogTee.h"   // Serial.* -> USB + plik /log.txt na karcie SD

// Przestrzeń nazw grupująca wszystkie stałe sprzętowe i parametry motocykla
namespace Config {
    // --- PINY SPRZĘTOWE ---
    constexpr uint8_t PIN_PWR_EN       = 10; // Główny klucz zasilania płytki T-HMI
    constexpr uint8_t PIN_TFT_BL       = 38; // Fizyczny pin podświetlenia ekranu w LilyGO T-HMI

    constexpr uint8_t PIN_I2C_SDA      = 43; // Linia danych magistrali Grove I2C (termometr/barometr)
    constexpr uint8_t PIN_I2C_SCL      = 44; // Linia zegara magistrali Grove I2C
    // Czytnik microSD na płytce T-HMI (tryb SD_MMC 1-bit, nie SPI)
    constexpr uint8_t PIN_SD_CLK       = 12;
    constexpr uint8_t PIN_SD_CMD       = 11;
    constexpr uint8_t PIN_SD_D0        = 13;
    constexpr int     SD_FREQ_KHZ      = 20000; // Domyślne 40 MHz bywa za szybkie dla T-HMI - karta nie jest wykrywana
    // true = karta, której nie da się odczytać (exFAT/NTFS), zostanie sformatowana na FAT32 - tylko raz na tym
    // urządzeniu (znacznik "sdFmtDone" w pamięci Flash). Kasuje całą zawartość karty!
    constexpr bool    SD_FORMAT_ONCE   = true;
    // Złącze Grove IO15/IO16: moduł CAN (SN65HVD230). Sterowanie MotoDash jest dotykowe - bez przycisków.
    constexpr uint8_t PIN_CAN_A        = 15; // CTX modułu CAN (kierunek pinów wykrywa się sam)
    constexpr uint8_t PIN_CAN_B        = 16; // CRX modułu CAN
    constexpr uint8_t PIN_GPS_A        = 17; // Złącze Grove GPS (UART) - który pin nadaje, wykrywa się samo
    constexpr uint8_t PIN_GPS_B        = 18;
    // true = moduł CAN podłączony (odczyt OBD-II), false = symulacja silnika (testy bez motocykla/auta)
    constexpr bool    CAN_ENABLED      = true;

    // --- EKRAN TANKOWANIA ---
    constexpr float         REFUEL_PROMPT_L   = 3.0f;  // Poniżej tej ilości paliwa [L] ekran Tankowania otwiera się sam po starcie
    constexpr unsigned long REFUEL_TIMEOUT_MS = 30000; // Bez naciśnięcia przycisku przez ten czas ekran wraca do jazdy [ms]

    // --- AUTOMATYCZNA KALIBRACJA SPALANIA (przy tankowaniu do pełna) ---
    constexpr float CAL_MIN_BURNED_L = 5.0f;  // Minimalne spalone paliwo od poprzedniego "do pełna", by kalibrować [L]
    constexpr float CAL_MAX_RATIO    = 1.30f; // Odrzucamy wynik, gdy różnica komputer/dystrybutor > 30% (pomyłka?)
    constexpr float CAL_FACTOR_MIN   = 0.70f; // Dopuszczalny zakres mnożnika kalibracji
    constexpr float CAL_FACTOR_MAX   = 1.30f;

    // --- CZUJNIKI CIŚNIENIA W OPONACH (TPMS, Bluetooth LE) ---
    // Tanie nakrętki TPMS mają numer 1-4 (zwykle na naklejce). Który numer jest na którym kole:
    constexpr uint8_t TPMS_FRONT_NO = 1;  // Numer czujnika na przednim kole (0 = brak czujnika)
    constexpr uint8_t TPMS_REAR_NO  = 2;  // Numer czujnika na tylnym kole  (0 = brak czujnika)
    // Albo konkretny adres czujnika z logu "[TPMS]" (ma pierwszeństwo przed numerem), np. "80:ea:ca:11:22:33"
    constexpr const char* TPMS_FRONT_MAC = "";
    constexpr const char* TPMS_REAR_MAC  = "";
    constexpr unsigned long TPMS_STALE_MS = 15UL * 60 * 1000; // Po tylu ms bez danych odczyt uznajemy za nieaktualny
    // Tryb rozpoznawania: wypisuje w Serial Monitorze surowe dane z pobliskich urządzeń BLE (silny sygnał).
    // Przydaje się, gdy czujnik nadaje w innym formacie - wyślij log, a dopiszemy dekoder. Potem ustaw false.
    constexpr bool TPMS_DEBUG_SCAN = true;
    constexpr int  TPMS_DEBUG_MIN_RSSI = -65; // Tylko urządzenia blisko płytki (połóż czujnik obok)
    // Identyfikator z obudowy czujnika (np. "C225B17B"). Każde ogłoszenie, którego adres lub dane zawierają
    // ten ciąg (w dowolnej kolejności bajtów), jest logowane jako "[TPMS-ID]" - bez filtra siły sygnału.
    constexpr const char* TPMS_DEBUG_ID = "C225B17B";

    // --- HOTSPOT WI-FI (ekran 5) ---
    constexpr const char* WIFI_AP_SSID = "MotoDash";    // Nazwa sieci widoczna w telefonie
    constexpr const char* WIFI_AP_PASS = "motodash123"; // Hasło (min. 8 znaków) - zmień na własne
    // Automatyczne wyłączenie hotspotu po ruszeniu. Na razie WYŁĄCZONE, bo symulacja CAN zawsze
    // podaje prędkość > 0. Po podłączeniu prawdziwego CAN ustaw true.
    constexpr bool  WIFI_OFF_WHEN_RIDING = false;
    constexpr int   WIFI_OFF_SPEED_KMH   = 10;

    // --- DANE TESTOWE ---
    // true = przy każdym starcie w baku zostaje TEST_FUEL_LEFT_L litrów (do testu automatycznego ekranu Tankowania).
    // PRZED JAZDĄ USTAW false, inaczej po każdym uruchomieniu stan baku będzie nadpisywany!
    constexpr bool  TEST_LOW_FUEL_ON_BOOT = true;
    constexpr float TEST_FUEL_LEFT_L      = 2.0f;  // Ile litrów zostaje w baku przy starcie
    constexpr float TEST_AVG_FUEL100      = 5.5f;  // Średnie spalanie z baku przyjęte w teście [L/100km]

    // --- STAŁE SILNIKA BENELLI TRK 702 ---
    constexpr float ENGINE_DISPLACEMENT = 0.698f; // Pojemność skokowa w litrach (698 cm3)
    constexpr float VOLUMETRIC_EFF      = 0.84f;  // Szacowana sprawność wolumetryczna silnika
    constexpr int   ENGINE_TEMP_WARN    = 104;    // Próg alarmowy przegrzania płynu chłodzącego [°C]

    // --- GPS i ZAPIS TRASY (GPX na karcie SD, katalog /trasy) ---
    constexpr int   GPX_INTERVAL_S   = 5;      // Punkt trasy najczęściej co tyle sekund...
    constexpr float GPX_MIN_MOVE_M   = 10.0f;  // ...o ile motocykl przejechał co najmniej tyle metrów
    constexpr float GPX_MAX_GAP_M    = 50.0f;  // Przy szybkiej jeździe punkt najpóźniej co tyle metrów
    constexpr float STOP_SPEED_KMH   = 3.0f;   // Poniżej tej prędkości motocykl stoi
    constexpr int   STOP_MIN_S       = 300;    // Postój zaznaczany na mapie od tylu sekund (5 min)
    constexpr long  TRIP_GAP_S       = 7200;   // Dłuższa przerwa (lub nowy dzień) = nowy plik trasy (2 h)
    constexpr int   ALT_GPS_MIN_SATS = 6;      // Wysokość z GPS używana do korekty barometru od tylu satelitów
    constexpr float ALT_GPS_MAX_HDOP = 2.0f;   // ...i przy dokładności HDOP nie gorszej niż ta
}
