#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "MotoState.h"

class SettingsScreen;
class TripsScreen;
class RefuelLog;
class FuelCalculator;
class TrackLogger;

// Hotspot Wi-Fi "MotoDash" ze stroną WWW: stan motocykla, historia tras (+ pobranie CSV) i edycja ustawień.
// Telefon łączy się z siecią MotoDash i otwiera http://192.168.4.1 (zwykle sam proponuje otwarcie strony).
// Wszystkie żądania obsługiwane są w loop() przez update() - bez osobnych wątków i bez wyścigów o MotoState.
class WifiPortal {
public:
    void init(MotoState* statePtr, SettingsScreen* settingsPtr, TripsScreen* tripsPtr,
              RefuelLog* refuelPtr, FuelCalculator* fuelPtr, TrackLogger* trackPtr);
    void update();  // Wywoływać w każdym obiegu loop(): obsługa żądań HTTP/DNS i automatyczne wyłączenie

    void start();   // Włącza hotspot i serwer WWW
    void stop();    // Wyłącza hotspot i radio Wi-Fi
    void toggle() { active ? stop() : start(); }

    bool   isActive() const          { return active; }
    bool   wasStoppedByRiding() const { return stoppedByRiding; }
    int    clientCount() const;      // Liczba podłączonych telefonów/komputerów
    String address() const;          // Adres strony, np. "http://192.168.4.1"

private:
    MotoState*      state    = nullptr;
    SettingsScreen* settings = nullptr;
    TripsScreen*    trips    = nullptr;
    RefuelLog*      refuels  = nullptr;
    FuelCalculator* fuel     = nullptr;
    TrackLogger*    track    = nullptr;

    WebServer server{80};
    DNSServer dns;
    bool active          = false;
    bool stoppedByRiding = false;

    void handleRoot();
    void handleStatus();
    void handleTrips();
    void handleCsv();
    void handleSaveSettings();
    void handleRefuels();
    void handleRefuelsCsv();
    void handleSetTime();
    void handleGpxList();   // Lista plików GPX z karty SD (JSON)
    void handleGpxFile();   // Pobranie jednego pliku GPX
    void handleNotFound();
};
