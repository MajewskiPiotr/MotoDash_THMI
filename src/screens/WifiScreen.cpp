#include "WifiScreen.h"
#include "Theme.h"
#include "Config.h"

// Karta statusu
static constexpr int CARD_X = 8, CARD_Y = 22, CARD_W = 304, CARD_H = 188;

void WifiScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    dirty = true;
    tft.fillScreen(BG_COLOR);
    tft.setTextColor(ACCENT_BLUE, BG_COLOR);
    tft.drawString("WI-FI / HOTSPOT", 10, 6, 1);
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    tft.drawString("[ACT]: wlacz / wylacz hotspot", 10, 218, 1);
}

void WifiScreen::update(TFT_eSPI& tft, const MotoState& state) {
    if (!portal) return;

    // Liczbę urządzeń sprawdzamy raz na sekundę
    unsigned long now = millis();
    if (!dirty && now - lastPollMs < 1000) return;
    lastPollMs = now;

    bool active  = portal->isActive();
    int  clients = portal->clientCount();
    if (!dirty && active == lastActive && clients == lastClients) return;
    dirty = false;
    lastActive  = active;
    lastClients = clients;

    tft.fillRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, 7, CARD_COLOR);
    tft.drawRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, 7, active ? ACCENT_GREEN : BORDER_COLOR);

    tft.setTextColor(active ? ACCENT_GREEN : TEXT_MUTED, CARD_COLOR);
    tft.drawCentreString(active ? "HOTSPOT WLACZONY" : "HOTSPOT WYLACZONY", 160, 34, 4);

    if (active) {
        tft.setTextColor(TEXT_MUTED, CARD_COLOR);
        tft.drawString("Siec:",  20, 78, 2);
        tft.drawString("Haslo:", 20, 102, 2);
        tft.drawString("Adres:", 20, 126, 2);
        tft.setTextColor(TFT_WHITE, CARD_COLOR);
        tft.drawString(Config::WIFI_AP_SSID, 84, 78, 2);
        tft.drawString(Config::WIFI_AP_PASS, 84, 102, 2);
        tft.setTextColor(ACCENT_BLUE, CARD_COLOR);
        tft.drawString(portal->address(), 84, 126, 2);

        tft.setTextColor(clients > 0 ? ACCENT_GREEN : TEXT_MUTED, CARD_COLOR);
        tft.drawCentreString(clients > 0 ? "Polaczone urzadzenia: " + String(clients)
                                         : "Czekam na polaczenie telefonu...", 160, 168, 2);
    } else {
        tft.setTextColor(TEXT_MUTED, CARD_COLOR);
        tft.drawCentreString("Kliknij ACTION, aby wlaczyc.", 160, 84, 2);
        tft.drawCentreString("Potem polacz telefon z siecia", 160, 112, 2);
        tft.drawCentreString(String("\"") + Config::WIFI_AP_SSID + "\" i otworz strone.", 160, 132, 2);
        if (portal->wasStoppedByRiding()) {
            tft.setTextColor(ACCENT_YELLOW, CARD_COLOR);
            tft.drawCentreString("Wylaczony automatycznie podczas jazdy", 160, 168, 2);
        }
    }
}

bool WifiScreen::onActionClick(MotoState& state) {
    if (portal) portal->toggle();
    dirty = true;
    return true; // Blokuje globalną zmianę jasności
}
