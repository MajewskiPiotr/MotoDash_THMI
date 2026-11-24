#include <Arduino.h>
#include "Config.h"
#include "MotoState.h"
#include "CanReader.h"
#include "EnvSensor.h"
#include "TpmsScanner.h"
#include "FuelCalculator.h"
#include "ScreenManager.h"
#include "RidingScreen.h"
#include "MotoStatsScreen.h"
#include "SettingsScreen.h"
#include "TripsScreen.h"
#include "TouchHandler.h"
#include "RefuelScreen.h"
#include "WifiPortal.h"
#include "RefuelLog.h"
#include "WifiScreen.h"
#include "GpsService.h"
#include "TrackLogger.h"

// --- INSTANCJE GLOBALNE SERWISÓW I MODELU STANU ---
MotoState       motoState;
CanReader       canReader;
EnvSensor       envSensor;
TpmsScanner     tpmsScanner;
GpsService      gpsService;
TrackLogger     trackLogger;   // Zapis trasy GPX na kartę SD
FuelCalculator  fuelCalculator;
ScreenManager   screenManager;
TouchHandler    touchHandler;

// --- INSTANCJE POSZCZEGÓLNYCH EKRANÓW ---
RefuelLog       refuelLog;     // Historia tankowań i samoczynna kalibracja
RefuelScreen    refuelScreen(&fuelCalculator, &screenManager, &refuelLog); // Nakładka (poza karuzelą)
RidingScreen    ridingScreen(&fuelCalculator, &refuelScreen);
MotoStatsScreen motoStatsScreen(&canReader);
SettingsScreen  settingsScreen(&screenManager);
TripsScreen     tripsScreen(&fuelCalculator);
WifiPortal      wifiPortal;
WifiScreen      wifiScreen(&wifiPortal);

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

    // Kalibracja dotyku: przy pierwszym uruchomieniu albo gdy trzymasz palec na ekranie podczas startu
    if (!touchHandler.isCalibrated() || touchHandler.isTouchedNow()) {
        touchHandler.calibrate(screenManager.display());
    }
    settingsScreen.setTouchHandler(&touchHandler);

    // 3. Rejestracja widoków w kolejności przełączania
    screenManager.registerScreen(&ridingScreen);
    screenManager.registerScreen(&motoStatsScreen);
    screenManager.registerScreen(&tripsScreen);
    screenManager.registerScreen(&wifiScreen);
    screenManager.registerScreen(&settingsScreen);

    // 4. Uruchomienie poszczególnych modułów sprzętowych i logiki
    canReader.init();
    envSensor.init();
    gpsService.init();
    tpmsScanner.init();
    fuelCalculator.init(motoState);
    tripsScreen.initSdAndLoad(motoState);
    refuelLog.init(fuelCalculator.odometerKm());
    refuelLog.setSdAvailable(tripsScreen.hasSd());
    LogSerial.attachSd(tripsScreen.hasSd()); // Od teraz log trafia też do /log.txt na karcie
    trackLogger.init(tripsScreen.hasSd());
    refuelLog.setTrackLogger(&trackLogger);
    wifiPortal.init(&motoState, &settingsScreen, &tripsScreen, &refuelLog, &fuelCalculator, &trackLogger); // Hotspot startuje dopiero z ekranu Wi-Fi

    // 5. Prawie pusty bak po uruchomieniu = prawdopodobnie jesteśmy na stacji -> pokaż ekran Tankowania
    if (motoState.fuelInTankL < Config::REFUEL_PROMPT_L) {
        refuelScreen.open(motoState, true);
    }
}

void loop() {
    // 1. Obsługa dotyku (jedyne sterowanie - złącze po przyciskach zajmuje moduł CAN)
    touchHandler.update(motoState, screenManager);

    // 2. Zbieranie aktualnych odczytów z motocykla i czujników
    canReader.update(motoState);
    gpsService.update(motoState);
    envSensor.update(motoState);
    tpmsScanner.update(motoState);

    // 3. Przeliczenie spalania i zasięgu na podstawie nowych odczytów
    fuelCalculator.update(motoState);

    // 3a. Zapis trasy GPX (gdy jest pozycja GPS i karta SD)
    trackLogger.update(motoState);

    // 4. Obsługa strony WWW, gdy hotspot jest włączony
    wifiPortal.update();

    // 5. Odświeżenie aktualnie wybranego widoku na ekranie
    screenManager.render(motoState);

    // 6. Zapis logu na kartę SD (co ~2 s)
    LogSerial.loop();
}
