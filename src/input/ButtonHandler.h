#pragma once
#include <OneButton.h>
#include "Config.h"
#include "MotoState.h"
#include "ScreenManager.h"

// Kontroler obsługujący fizyczne przyciski i kierujący zdarzenia do aktywnego ekranu.
class ButtonHandler {
public:
    void init(MotoState* statePtr, ScreenManager* screenMgrPtr); // Rejestruje callbacki przycisków
    void tick(); // Metoda odpytująca stan przycisków (musi być wywoływana w każdej pętli loop)

private:
    // Tworzymy instancje przycisków z włączonym wewnętrznym podciąganiem do plusa (INPUT_PULLUP)
    OneButton btnMode   = OneButton(Config::PIN_BTN_MODE, true, true);
    OneButton btnAction = OneButton(Config::PIN_BTN_ACTION, true, true);
};
