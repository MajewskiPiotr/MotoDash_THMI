#pragma once
#include <Arduino.h>
#include "MotoState.h"

// Czujniki otoczenia na magistrali I2C złącza Grove (GPIO 43/44):
//  - AHT20 / DHT20 (0x38): temperatura,
//  - BME280 / BMP280 (0x76 lub 0x77): ciśnienie -> wysokość n.p.m., temperatura zapasowa.
// Oba mogą być podłączone jednocześnie (różne adresy). Temperatura z AHT20 ma pierwszeństwo,
// bo BME280 lekko się nagrzewa od własnej elektroniki.
class EnvSensor {
public:
    void init();                   // Uruchamia I2C i wykrywa podłączone czujniki
    void update(MotoState& state); // Odczyt co 2 s

private:
    static constexpr uint8_t AHT20_ADDR = 0x38;

    unsigned long lastReadMs = 0;
    bool    ahtReady  = false;
    uint8_t bmeAddr   = 0;     // 0 = brak barometru
    bool    hasHumid  = false; // true = BME280, false = BMP280 (bez wilgotności)

    // Kalibracja altymetru: ciśnienie odniesienia liczone przy pierwszym odczycie
    // tak, aby wysokość = "Wysokość startowa" z ustawień.
    float seaLevelHpa  = 0.0f;
    int   calibBaseAlt = -100000;

    // Współczynniki kalibracyjne BME280 (z pamięci czujnika)
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    int32_t  tFine = 0;

    bool readAht20Temp(float& tempOut);
    bool initBme(uint8_t addr);
    bool readBme(float& tempC, float& hPa);
    bool readRegs(uint8_t reg, uint8_t* buf, uint8_t len);
    bool writeReg(uint8_t reg, uint8_t val);
};
