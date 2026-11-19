#pragma once
#include <OneButton.h>
#include "Config.h"
#include "MotoState.h"
#include "ScreenManager.h"

// Kontroler obsĹ‚ugujÄ…cy fizyczne przyciski i kierujÄ…cy zdarzenia do aktywnego ekranu.
class ButtonHandler {
public:
    void init(MotoState* statePtr, ScreenManager* screenMgrPtr); // Rejestruje callbacki przyciskĂłw
    void tick(); // Metoda odpytujÄ…ca stan przyciskĂłw (musi byÄ‡ wywoĹ‚ywana w kaĹĽdej pÄ™tli loop)

private:
    // Tworzymy instancje przyciskĂłw z wĹ‚Ä…czonym wewnÄ™trznym podciÄ…ganiem do plusa (INPUT_PULLUP)
    OneButton btnMode   = OneButton(Config::PIN_BTN_MODE, true, true);
    OneButton btnAction = OneButton(Config::PIN_BTN_ACTION, true, true);
};
