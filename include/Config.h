#pragma once          // Zabezpiecza plik przed wielokrotnym dołączeniem podczas kompilacji
#include <Arduino.h>  // Podstawowe typy danych (uint8_t, String itp.)

// Przestrzeń nazw grupująca wszystkie stałe sprzętowe i parametry motocykla
namespace Config {
    // --- PINY SPRZĘTOWE ---
constexpr uint8_t PIN_PWR_EN     = 10; // Główny klucz zasilania płytki T-HMI
constexpr uint8_t PIN_TFT_BL     = 38; // Fizyczny pin podświetlenia ekranu w LilyGO T-HMI

    constexpr uint8_t PIN_I2C_SDA      = 43; // Linia danych magistrali Grove I2C (termometr/barometr)
    constexpr uint8_t PIN_I2C_SCL      = 44; // Linia zegara magistrali Grove I2C
    constexpr uint8_t PIN_BTN_MODE     = 15; // Niebieski przycisk nawigacji (GPIO 15)
    constexpr uint8_t PIN_BTN_ACTION   = 16; // Czerwony przycisk akcji (GPIO 16)

    // --- STAŁE SILNIKA BENELLI TRK 702 ---
    constexpr float ENGINE_DISPLACEMENT = 0.698f; // Pojemność skokowa w litrach (698 cm3)
    constexpr float VOLUMETRIC_EFF      = 0.84f;  // Szacowana sprawność wolumetryczna silnika
    constexpr int   ENGINE_TEMP_WARN    = 104;    // Próg alarmowy przegrzania płynu chłodzącego [°C]
}
