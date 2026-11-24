#pragma once
#include "Screen.h"
#include "WifiPortal.h"

// Ekran 5: Hotspot Wi-Fi. ACTION włącza / wyłącza hotspot "MotoDash".
// Pokazuje nazwę sieci, hasło, adres strony i liczbę podłączonych urządzeń.
class WifiScreen : public Screen {
public:
    explicit WifiScreen(WifiPortal* p) : portal(p) {}

    void onEnter(TFT_eSPI& tft, const MotoState& state) override;
    void update(TFT_eSPI& tft, const MotoState& state) override;
    bool onTap(MotoState& state, int x, int y) override; // Stuknięcie w kartę: włącz / wyłącz hotspot

private:
    WifiPortal* portal;
    bool dirty       = true;  // Przerysuj kartę statusu
    bool lastActive  = false;
    int  lastClients = -1;
    unsigned long lastPollMs = 0;
};
