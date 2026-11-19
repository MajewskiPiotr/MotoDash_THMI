#include "EnvSensor.h"
#include "Config.h"
#include <Wire.h>

void EnvSensor::init() {
    Wire.begin(Config::PIN_I2C_SDA, Config::PIN_I2C_SCL); // Uruchamia I2C na pinach Grove
    Wire.setClock(100000);
    Serial.println("[EnvSensor] Inicjalizacja czujnika I2C Grove");
}

void EnvSensor::update(MotoState& state) {
    if (millis() - lastReadMs < 2000) return;
    lastReadMs = millis();

    // Tymczasowe dane testowe (docelowo odczyt z SHT30 / ENV III)
    state.ambientTempC = 19.2f;
    state.altitudeM    = (float)state.baseAltitudeM + 25.0f;
}
