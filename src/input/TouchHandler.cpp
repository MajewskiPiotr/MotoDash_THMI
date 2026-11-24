#include "TouchHandler.h"
#include "Config.h"
#include "Theme.h"
#include "SmoothText.h"
#include <Preferences.h>

void TouchHandler::init() {
    SPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin();
    ts.setRotation(1); // Poziome ułożenie ekranu (320x240)
    loadCalibration();
    Serial.println(calibrated ? "[Touch] Panel dotykowy gotowy (kalibracja z pamieci)"
                              : "[Touch] Panel dotykowy bez kalibracji - kalibracja przy starcie");
}

// ---------------------------------------------------------------------------
// Kalibracja
// ---------------------------------------------------------------------------

void TouchHandler::loadCalibration() {
    Preferences p;
    if (!p.begin("motodash", true)) return;
    calibrated = p.getBool("tcOk", false);
    if (calibrated) {
        axisX = p.getUChar("tcAxis", 0);
        ax = p.getFloat("tcAx", ax); bx = p.getFloat("tcBx", bx);
        ay = p.getFloat("tcAy", ay); by = p.getFloat("tcBy", by);
    }
    p.end();
}

void TouchHandler::saveCalibration() {
    Preferences p;
    if (!p.begin("motodash", false)) return;
    p.putBool("tcOk", true);
    p.putUChar("tcAxis", axisX);
    p.putFloat("tcAx", ax); p.putFloat("tcBx", bx);
    p.putFloat("tcAy", ay); p.putFloat("tcBy", by);
    p.end();
}

void TouchHandler::rawToScreen(int rx, int ry, int& sx, int& sy) const {
    int rawForX = axisX == 0 ? rx : ry;
    int rawForY = axisX == 0 ? ry : rx;
    sx = constrain((int)(ax * rawForX + bx), 0, 319);
    sy = constrain((int)(ay * rawForY + by), 0, 239);
}

bool TouchHandler::isTouchedNow() {
    if (!ts.touched()) return false;
    return ts.getPoint().z >= MIN_Z;
}

// Czeka na dotknięcie, uśrednia odczyty do oderwania palca (max 30 s)
bool TouchHandler::waitPress(int& rawX, int& rawY) {
    unsigned long t0 = millis();
    while (isTouchedNow()) { delay(10); if (millis() - t0 > 30000) return false; } // Najpierw puść
    while (!isTouchedNow()) { delay(10); if (millis() - t0 > 30000) return false; }
    long sx = 0, sy = 0; int n = 0, gone = 0;
    while (gone < 4) {
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            if (p.z >= MIN_Z) { sx += p.x; sy += p.y; n++; gone = 0; }
        } else {
            gone++;
        }
        delay(10);
        if (millis() - t0 > 30000) return false;
    }
    if (n < 3) return false;
    rawX = sx / n; rawY = sy / n;
    return true;
}

static void drawTarget(TFT_eSPI& tft, int x, int y, uint16_t c) {
    tft.drawCircle(x, y, 10, c);
    tft.drawLine(x - 16, y, x + 16, y, c);
    tft.drawLine(x, y - 16, x, y + 16, c);
}

void TouchHandler::calibrate(TFT_eSPI& tft) {
    const int PX[3] = { 30, 290, 30 };
    const int PY[3] = { 30, 30, 210 };
    for (int attempt = 0; attempt < 3; attempt++) {
        int rx[3], ry[3];
        bool ok = true;
        for (int i = 0; i < 3 && ok; i++) {
            tft.fillScreen(BG_COLOR);
            tft.setTextColor(TFT_WHITE, BG_COLOR);
            SmoothText::str(tft, "KALIBRACJA DOTYKU", 160, 92, 2, SmoothText::CENTER);
            tft.setTextColor(TEXT_MUTED, BG_COLOR);
            SmoothText::str(tft, "Dotknij środka krzyżyka (" + String(i + 1) + " z 3)", 160, 118, 2, SmoothText::CENTER);
            drawTarget(tft, PX[i], PY[i], ACCENT_ORANGE);
            ok = waitPress(rx[i], ry[i]);
            Serial.printf("[Touch] Punkt %d: surowe %d, %d\n", i + 1, rx[i], ry[i]);
        }
        if (ok) {
            // Oś surowa, która zmienia się najbardziej przy ruchu w poziomie, to oś X ekranu
            int dxX = abs(rx[1] - rx[0]), dxY = abs(ry[1] - ry[0]);
            axisX = dxX >= dxY ? 0 : 1;
            int rX0 = axisX == 0 ? rx[0] : ry[0], rX1 = axisX == 0 ? rx[1] : ry[1];
            int rY0 = axisX == 0 ? ry[0] : rx[0], rY2 = axisX == 0 ? ry[2] : rx[2];
            if (abs(rX1 - rX0) > 400 && abs(rY2 - rY0) > 300) {
                ax = (float)(PX[1] - PX[0]) / (rX1 - rX0); bx = PX[0] - ax * rX0;
                ay = (float)(PY[2] - PY[0]) / (rY2 - rY0); by = PY[0] - ay * rY0;
                calibrated = true;
                saveCalibration();
                Serial.printf("[Touch] Kalibracja zapisana (os X = surowe %c)\n", axisX == 0 ? 'x' : 'y');
                tft.fillScreen(BG_COLOR);
                tft.setTextColor(ACCENT_GREEN, BG_COLOR);
                SmoothText::str(tft, "Kalibracja zapisana", 160, 110, 2, SmoothText::CENTER);
                delay(800);
                return;
            }
        }
        tft.fillScreen(BG_COLOR);
        tft.setTextColor(ACCENT_YELLOW, BG_COLOR);
        SmoothText::str(tft, "Nie udało się - jeszcze raz", 160, 110, 2, SmoothText::CENTER);
        delay(1200);
    }
}

// ---------------------------------------------------------------------------
// Gesty
// ---------------------------------------------------------------------------

bool TouchHandler::readPoint(int& sx, int& sy) {
    if (!ts.touched()) return false;
    TS_Point p = ts.getPoint();
    if (p.z < MIN_Z) return false;
    rawToScreen(p.x, p.y, sx, sy);
    return true;
}

void TouchHandler::toggleBrightness(MotoState& state) {
    state.nightBrightness = !state.nightBrightness;
    analogWrite(Config::PIN_TFT_BL, state.nightBrightness ? 70 : 255);
}

void TouchHandler::update(MotoState& state, ScreenManager& mgr) {
    unsigned long now = millis();
    if (now - lastPollMs < 20) return;
    lastPollMs = now;

    int x, y;
    if (readPoint(x, y)) {
        releaseCnt = 0;
        if (!down) {
            down = true; longFired = false;
            startX = lastX = x; startY = lastY = y;
            downMs = now;
        } else {
            lastX = x; lastY = y;
        }
        // Przytrzymanie: palec w miejscu przez LONG_MS - akcja od razu, bez czekania na oderwanie
        if (!longFired && now - downMs >= LONG_MS &&
            abs(lastX - startX) < TAP_MOVE_PX && abs(lastY - startY) < TAP_MOVE_PX) {
            longFired = true;
            Screen* s = mgr.getActiveScreen();
            Serial.printf("[Touch] Przytrzymanie %d,%d\n", startX, startY);
            if (s) s->onLongTouch(state, startX, startY);
        }
        return;
    }

    if (!down) return;
    if (++releaseCnt < RELEASE_POLLS) return; // Krótkie przerwy w odczycie to nie oderwanie palca
    down = false;
    if (longFired) return;

    int dx = lastX - startX, dy = lastY - startY;
    if (abs(dx) >= SWIPE_PX && abs(dx) > abs(dy) * 3 / 2) {
        Serial.println(dx < 0 ? "[Touch] Przesuniecie w lewo -> nastepny ekran" : "[Touch] Przesuniecie w prawo -> poprzedni ekran");
        if (dx < 0) mgr.nextScreen(state); else mgr.prevScreen(state);
        return;
    }
    if (abs(dx) < TAP_MOVE_PX && abs(dy) < TAP_MOVE_PX) {
        Serial.printf("[Touch] Stukniecie %d,%d\n", startX, startY);
        Screen* s = mgr.getActiveScreen();
        if (s && s->onTap(state, startX, startY)) return;
        if (startY < 20) toggleBrightness(state); // Górny pasek: jasność dzień / noc
    }
}
