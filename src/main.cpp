#include <Arduino.h>
#include "Config.h"
#include "MotoState.h"
#include "CanReader.h"
#include "EnvSensor.h"
#include "TpmsScanner.h"
#include "FuelCalculator.h"
#include "ButtonHandler.h"
#include "ScreenManager.h"
#include "RidingScreen.h"
#include "MotoStatsScreen.h"
#include "SettingsScreen.h"
#include "AnaliticScreen.h"
#include "TouchHandler.h"
#include "RefuelScreen.h"

// --- INSTANCJE GLOBALNE SERWISÓW I MODELU STANU ---
MotoState       motoState;
CanReader       canReader;
EnvSensor       envSensor;
TpmsScanner     tpmsScanner;
FuelCalculator  fuelCalculator;
ButtonHandler   buttonHandler;
ScreenManager   screenManager;
TouchHandler    touchHandler;

// --- INSTANCJE POSZCZEGÓLNYCH EKRANÓW ---
RefuelScreen    refuelScreen(&fuelCalculator, &screenManager); // Nakładka (poza karuzelą)
RidingScreen    ridingScreen(&fuelCalculator, &refuelScreen);
MotoStatsScreen motoStatsScreen(&canReader);
SettingsScreen  settingsScreen(&screenManager);
AnaliticScreen  analiticScreen;

void setup() {
    Serial.begin(115200);
    touchHandler.init();

    // 1. Wczytanie zapisanych ustawień użytkownika z nieulotnej pamięci Flash
    settingsScreen.loadFromNvs(motoState);

    // 2. Inicjalizacja ekranu
    screenManager.init();

    // >>> TUTAJ WYWOŁUJEMY ANIMACJĘ POWITALNĄ <<<
    // (Odbywa się zanim zarejestrujemy i wyrenderujemy ekrany jazdy)
    screenManager.playBootAnimation();

    // 3. Rejestracja widoków w kolejności przełączania
    screenManager.registerScreen(&ridingScreen);
    screenManager.registerScreen(&motoStatsScreen);
    screenManager.registerScreen(&analiticScreen);
    screenManager.registerScreen(&settingsScreen);

    // 4. Uruchomienie poszczególnych modułów sprzętowych i logiki
    canReader.init();
    envSensor.init();
    tpmsScanner.init();
    fuelCalculator.init(motoState);
    buttonHandler.init(&motoState, &screenManager);
    analiticScreen.initSdAndLoad(motoState);
}

void loop() {
    // 1. Obsługa przycisków i dotyku
    buttonHandler.tick();
    touchHandler.update(motoState, screenManager);

    // 2. Zbieranie aktualnych odczytów z motocykla i czujników
    canReader.update(motoState);
    envSensor.update(motoState);
    tpmsScanner.update(motoState);

    // 3. Przeliczenie spalania i zasięgu na podstawie nowych odczytów
    fuelCalculator.update(motoState);

    // 4. Odświeżenie aktualnie wybranego widoku na ekranie
    screenManager.render(motoState);
}
