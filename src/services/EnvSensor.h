#pragma once
#include <Arduino.h>
#include "MotoState.h"

// Serwis obsługujący zewnętrzny czujnik temperatury AHT20 / DHT20 (adres I2C: 0x38) na złączu Grove.
class EnvSensor {
public:
    void init();                   // Uruchamia magistralę I2C (GPIO 43/44) i inicjuje czujnik AHT20
    void update(MotoState& state); // Cyklicznie odczytuje temperaturę otoczenia co 2 sekundy

private:
    static constexpr uint8_t AHT20_ADDR = 0x38; // Fabryczny adres I2C czujników AHT20 / AHT21 / DHT20

    unsigned long lastReadMs  = 0;     // Znacznik czasu ostatniego pomiaru [ms]
    bool          sensorReady = false; // Flaga potwierdzająca gotowość czujnika do pracy

    bool readAht20Temp(float& tempOut); // Wysyła żądanie pomiaru i przelicza surowe bajty na °C
};
