#include "EnvSensor.h"
#include "Config.h"
#include <Wire.h> // Wbudowana biblioteka ESP32 do komunikacji po magistrali I2C

void EnvSensor::init() {
    // 1. Uruchamiamy magistralę I2C na fizycznych pinach portu Grove płytki LilyGO T-HMI (SDA=43, SCL=44)
    Wire.begin(Config::PIN_I2C_SDA, Config::PIN_I2C_SCL);
    Wire.setClock(100000); // Taktowanie 100 kHz zapewniające stabilną pracę na przewodzie

    delay(40); // Czujnik AHT20 potrzebuje 40 ms po podaniu zasilania na uruchomienie wewnętrznej logiki

    // 2. Wysyłamy komendę kalibracji/inicjalizacji czujnika AHT20 (0xBE, 0x08, 0x00)
    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xBE);
    Wire.write(0x08);
    Wire.write(0x00);

    if (Wire.endTransmission() == 0) {
        sensorReady = true;
        delay(10); // Krótka pauza na zakończenie kalibracji
        Serial.println("[EnvSensor] Czujnik AHT20 (0x38) zainicjalizowany poprawnie!");
    } else {
        sensorReady = false;
        Serial.println("[EnvSensor] UWAGA: Brak odpowiedzi czujnika AHT20 pod adresem 0x38!");
    }
}

void EnvSensor::update(MotoState& state) {
    // Odczytujemy temperaturę spokojnie co 2 sekundy (2000 ms)
    if (millis() - lastReadMs < 2000) return;
    lastReadMs = millis();

    if (!sensorReady) return;

    float measuredTemp = 0.0f;
    // Pobieramy aktualny pomiar i wpisujemy do współdzielonego obiektu MotoState
    if (readAht20Temp(measuredTemp)) {
        state.ambientTempC = measuredTemp;
        Serial.println("[EnvSensor] Temperatura otoczenia: " + String(state.ambientTempC, 1) + " C");
    } else {
        Serial.println("[EnvSensor] Blad odczytu danych z czujnika AHT20");
    }
}

// Odczyt temperatury bezpośrednio z rejestrów czujnika AHT20 / DHT20
bool EnvSensor::readAht20Temp(float& tempOut) {
    // 1. Wysyłamy rozkaz wyzwolenia pojedynczego pomiaru (0xAC, 0x33, 0x00)
    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xAC);
    Wire.write(0x33);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    // 2. Według noty katalogowej AHT20 pomiar trwa maksymalnie 80 ms
    delay(80);

    // 3. Odbieramy 6 bajtów odpowiedzi:
    //: Status | [1..2]: Wilgotność |: Wilgotność (4 bity) + Temperatura (4 bity) | [4..5]: Temperatura
    if (Wire.requestFrom(AHT20_ADDR, (uint8_t)6) != 6) {
        return false;
    }

    uint8_t status = Wire.read(); // Bajt statusu (bit 7 == 0 oznacza, że pomiar jest gotowy)
    Wire.read();                  // Pomijamy starszy bajt wilgotności
    Wire.read();                  // Pomijamy środkowy bajt wilgotności

    uint8_t b3 = Wire.read();     // Młodsze 4 bity tego bajtu to najstarsze bity temperatury
    uint8_t b4 = Wire.read();     // Środkowy bajt temperatury
    uint8_t b5 = Wire.read();     // Najmłodszy bajt temperatury

    // Jeśli bit zajętości (0x80) jest nadal ustawiony, pomiar się nie zakończył
    if (status & 0x80) {
        return false;
    }

    // 4. Składamy 20-bitową wartość surową temperatury z bajtów b3, b4, b5
    uint32_t rawTemp = ((uint32_t)(b3 & 0x0F) << 16) | ((uint32_t)b4 << 8) | (uint32_t)b5;

    // 5. Przeliczamy na stopnie Celsjusza według oficjalnego wzoru producenta Aosong AHT20:
    // T [°C] = (rawTemp / 2^20) * 200 - 50
    tempOut = ((float)rawTemp / 1048576.0f) * 200.0f - 50.0f;

    return true;
}
