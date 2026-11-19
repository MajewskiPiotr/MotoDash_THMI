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


// --- INSTANCJE GLOBALNE SERWISĂ“W I MODELU STANU ---
MotoState       motoState;
CanReader       canReader;
EnvSensor       envSensor;
TpmsScanner     tpmsScanner;
FuelCalculator  fuelCalculator;
ButtonHandler   buttonHandler;
ScreenManager   screenManager;
TouchHandler touchHandler;

// --- INSTANCJE POSZCZEGĂ“LNYCH EKRANĂ“W ---
RidingScreen    ridingScreen(&fuelCalculator);
MotoStatsScreen motoStatsScreen(&canReader);
SettingsScreen  settingsScreen(&screenManager);
AnaliticScreen analiticScreen;

void setup() {
    Serial.begin(115200); // Uruchamia komunikacjÄ™ z konsolÄ… debugowania
touchHandler.init();
    // 1. Wczytanie zapisanych ustawieĹ„ uĹĽytkownika z nieulotnej pamiÄ™ci Flash
    settingsScreen.loadFromNvs(motoState);

    // 2. Inicjalizacja ekranu i rejestracja widokĂłw w kolejnoĹ›ci przeĹ‚Ä…czania
    screenManager.init();
    screenManager.registerScreen(&ridingScreen);
    screenManager.registerScreen(&motoStatsScreen);
    screenManager.registerScreen(&analiticScreen);
    screenManager.registerScreen(&settingsScreen);

    // 3. Uruchomienie poszczegĂłlnych moduĹ‚Ăłw sprzÄ™towych i logiki
    canReader.init();
    envSensor.init();
    tpmsScanner.init();
    fuelCalculator.init(motoState);
    buttonHandler.init(&motoState, &screenManager);
    analiticScreen.initSdAndLoad(motoState);

}

void loop() {
    // 1. ObsĹ‚uga przyciskĂłw (wykonywana na samym poczÄ…tku dla maksymalnej responsywnoĹ›ci)
    buttonHandler.tick();
    touchHandler.update(motoState, screenManager);
    // 2. Zbieranie aktualnych odczytĂłw z motocykla i czujnikĂłw
    canReader.update(motoState);
    envSensor.update(motoState);
    tpmsScanner.update(motoState);

    // 3. Przeliczenie spalania i zasiÄ™gu na podstawie nowych odczytĂłw
    fuelCalculator.update(motoState);

    // 4. OdĹ›wieĹĽenie aktualnie wybranego widoku na ekranie
    screenManager.render(motoState);
}
