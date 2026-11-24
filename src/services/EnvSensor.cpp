#include "EnvSensor.h"
#include "Config.h"
#include <Wire.h>
#include <math.h>

void EnvSensor::init() {
    Wire.begin(Config::PIN_I2C_SDA, Config::PIN_I2C_SCL);
    Wire.setClock(100000);
    delay(40); // AHT20 potrzebuje ~40 ms po włączeniu zasilania

    // --- AHT20 / DHT20 ---
    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xBE); Wire.write(0x08); Wire.write(0x00);
    ahtReady = (Wire.endTransmission() == 0);
    if (ahtReady) {
        delay(10);
        Serial.println("[EnvSensor] AHT20/DHT20 (0x38) wykryty");
    } else {
        Serial.println("[EnvSensor] Brak AHT20/DHT20 (0x38)");
    }

    // --- BME280 / BMP280: próbujemy oba możliwe adresy ---
    if (!initBme(0x76) && !initBme(0x77)) {
        Serial.println("[EnvSensor] Brak BME280/BMP280 (0x76/0x77)");
    }
}

bool EnvSensor::writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(bmeAddr);
    Wire.write(reg); Wire.write(val);
    return Wire.endTransmission() == 0;
}

bool EnvSensor::readRegs(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(bmeAddr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(bmeAddr, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

bool EnvSensor::initBme(uint8_t addr) {
    bmeAddr = addr;
    uint8_t id = 0;
    if (!readRegs(0xD0, &id, 1) || (id != 0x60 && id != 0x58)) {
        bmeAddr = 0;
        return false;
    }
    hasHumid = (id == 0x60);

    writeReg(0xE0, 0xB6); // miękki reset
    delay(5);

    uint8_t c[24];
    if (!readRegs(0x88, c, 24)) { bmeAddr = 0; return false; }
    T1 = c[0]  | (c[1]  << 8); T2 = c[2]  | (c[3]  << 8); T3 = c[4]  | (c[5]  << 8);
    P1 = c[6]  | (c[7]  << 8); P2 = c[8]  | (c[9]  << 8); P3 = c[10] | (c[11] << 8);
    P4 = c[12] | (c[13] << 8); P5 = c[14] | (c[15] << 8); P6 = c[16] | (c[17] << 8);
    P7 = c[18] | (c[19] << 8); P8 = c[20] | (c[21] << 8); P9 = c[22] | (c[23] << 8);

    if (hasHumid) writeReg(0xF2, 0x01); // wilgotność x1 (musi być przed 0xF4)
    writeReg(0xF5, 0xA8);               // odczyt co 1 s, filtr IIR x4 (wygładza podmuchy)
    writeReg(0xF4, 0x57);               // temp x2, ciśnienie x16, tryb ciągły

    Serial.printf("[EnvSensor] %s wykryty pod adresem 0x%02X\n", hasHumid ? "BME280" : "BMP280", addr);
    return true;
}

// Wzory kompensacji z noty katalogowej Bosch BME280 (wersja całkowitoliczbowa)
bool EnvSensor::readBme(float& tempC, float& hPa) {
    uint8_t d[6];
    if (!readRegs(0xF7, d, 6)) return false;
    int32_t adcP = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adcT = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    if (adcT == 0x80000 || adcP == 0x80000) return false; // pomiar jeszcze niegotowy

    int32_t v1 = ((((adcT >> 3) - ((int32_t)T1 << 1))) * ((int32_t)T2)) >> 11;
    int32_t v2 = (((((adcT >> 4) - ((int32_t)T1)) * ((adcT >> 4) - ((int32_t)T1))) >> 12) * ((int32_t)T3)) >> 14;
    tFine = v1 + v2;
    tempC = ((tFine * 5 + 128) >> 8) / 100.0f;

    int64_t p1 = ((int64_t)tFine) - 128000;
    int64_t p2 = p1 * p1 * (int64_t)P6;
    p2 = p2 + ((p1 * (int64_t)P5) << 17);
    p2 = p2 + (((int64_t)P4) << 35);
    p1 = ((p1 * p1 * (int64_t)P3) >> 8) + ((p1 * (int64_t)P2) << 12);
    p1 = (((((int64_t)1) << 47) + p1)) * ((int64_t)P1) >> 33;
    if (p1 == 0) return false;
    int64_t p = 1048576 - adcP;
    p = (((p << 31) - p2) * 3125) / p1;
    p1 = (((int64_t)P9) * (p >> 13) * (p >> 13)) >> 25;
    p2 = (((int64_t)P8) * p) >> 19;
    p = ((p + p1 + p2) >> 8) + (((int64_t)P7) << 4);
    hPa = (float)((uint32_t)p) / 25600.0f; // Pa*256 -> hPa
    return hPa > 300.0f && hPa < 1100.0f;
}

void EnvSensor::update(MotoState& state) {
    if (millis() - lastReadMs < 2000) return;
    lastReadMs = millis();

    bool tempFromAht = false;
    if (ahtReady) {
        float t;
        if (readAht20Temp(t)) {
            state.ambientTempC = t;
            tempFromAht = true;
        } else {
            Serial.println("[EnvSensor] Blad odczytu AHT20");
        }
    }

    if (bmeAddr) {
        float t, hPa;
        if (readBme(t, hPa)) {
            if (!tempFromAht) state.ambientTempC = t;
            state.pressureHpa = hPa;
            state.baroOk = true;

            // (Re)kalibracja: przy pierwszym odczycie lub po zmianie "Wysokości startowej"
            if (calibBaseAlt != state.baseAltitudeM) {
                calibBaseAlt = state.baseAltitudeM;
                seaLevelHpa  = hPa / powf(1.0f - calibBaseAlt / 44330.0f, 5.255f);
                Serial.printf("[EnvSensor] Altymetr skalibrowany: %d m, %.1f hPa (QNH %.1f)\n",
                              calibBaseAlt, hPa, seaLevelHpa);
            }
            state.altitudeM = 44330.0f * (1.0f - powf(hPa / seaLevelHpa, 0.1903f));
        } else {
            state.baroOk = false;
            Serial.println("[EnvSensor] Blad odczytu BME280");
        }
    }
}

// Odczyt temperatury z AHT20 / DHT20
bool EnvSensor::readAht20Temp(float& tempOut) {
    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xAC); Wire.write(0x33); Wire.write(0x00);
    if (Wire.endTransmission() != 0) return false;
    delay(80); // pomiar trwa do 80 ms

    if (Wire.requestFrom(AHT20_ADDR, (uint8_t)6) != 6) return false;
    uint8_t status = Wire.read();
    Wire.read(); Wire.read();
    uint8_t b3 = Wire.read(), b4 = Wire.read(), b5 = Wire.read();
    if (status & 0x80) return false; // pomiar niegotowy

    uint32_t raw = ((uint32_t)(b3 & 0x0F) << 16) | ((uint32_t)b4 << 8) | b5;
    tempOut = ((float)raw / 1048576.0f) * 200.0f - 50.0f;
    return true;
}
