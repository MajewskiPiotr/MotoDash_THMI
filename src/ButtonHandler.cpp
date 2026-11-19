#include "ButtonHandler.h"
#include <Arduino.h>

static MotoState*     s_state     = nullptr;
static ScreenManager* s_screenMgr = nullptr;

void ButtonHandler::init(MotoState* statePtr, ScreenManager* screenMgrPtr) {
    s_state     = statePtr;
    s_screenMgr = screenMgrPtr;

    pinMode(Config::PIN_BTN_MODE, INPUT_PULLUP);
    pinMode(Config::PIN_BTN_ACTION, INPUT_PULLUP);

    // --- KRĂ“TKIE KLIKNIÄCIE PRZYCISKU MODE ---
    btnMode.attachClick([]() {
        if (!s_state || !s_screenMgr) return;
        Screen* active = s_screenMgr->getActiveScreen();
        // Najpierw pytamy aktywny ekran (np. SettingsScreen), czy chce przejÄ…Ä‡ klikniÄ™cie.
        // JeĹ›li zwrĂłci false, przeĹ‚Ä…czamy na kolejny ekran w karuzeli.
        if (active && active->onModeClick(*s_state)) return;
        s_screenMgr->nextScreen(*s_state);
    });

    // --- DĹUGIE PRZYTRZYMANIE PRZYCISKU MODE ---
    btnMode.attachLongPressStart([]() {
        if (!s_state || !s_screenMgr) return;
        Screen* active = s_screenMgr->getActiveScreen();
        if (active) active->onModeLongPress(*s_state);
    });

    // --- KRĂ“TKIE KLIKNIÄCIE PRZYCISKU ACTION ---
    btnAction.attachClick([]() {
        if (!s_state || !s_screenMgr) return;
        Screen* active = s_screenMgr->getActiveScreen();
        // JeĹ›li aktywny ekran (np. SettingsScreen) obsĹ‚uĹĽy klikniÄ™cie (+), koĹ„czymy.
        // W przeciwnym razie wykonujemy akcjÄ™ globalnÄ… (zmiana jasnoĹ›ci DzieĹ„ / Noc).
        if (active && active->onActionClick(*s_state)) return;

        s_state->nightBrightness = !s_state->nightBrightness;
        analogWrite(Config::PIN_TFT_BL, s_state->nightBrightness ? 70 : 255);
    });

    // --- DĹUGIE PRZYTRZYMANIE PRZYCISKU ACTION ---
    btnAction.attachLongPressStart([]() {
        if (!s_state || !s_screenMgr) return;
        Screen* active = s_screenMgr->getActiveScreen();
        if (active) active->onActionLongPress(*s_state);
    });
}

void ButtonHandler::tick() {
    // Sprawdza stan obu przyciskĂłw w kaĹĽdym obiegu pÄ™tli gĹ‚Ăłwnej
    btnMode.tick();
    btnAction.tick();
}
