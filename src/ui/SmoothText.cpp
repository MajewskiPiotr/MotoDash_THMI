#include "SmoothText.h"

namespace SmoothText {

void draw(TFT_eSPI& tft, const uint8_t* font, const String& text,
          int x, int y, int w, int h, Align align, uint16_t fg, uint16_t bg) {
    TFT_eSprite spr(&tft);
    spr.setColorDepth(16);
    if (!spr.createSprite(w, h)) { // Brak pamięci - rysujemy wprost na ekranie (może mignąć)
        tft.fillRect(x, y, w, h, bg);
        tft.loadFont(font);
        tft.setTextColor(fg, bg);
        tft.setTextDatum(align == LEFT ? TL_DATUM : align == CENTER ? TC_DATUM : TR_DATUM);
        tft.drawString(text, align == LEFT ? x : align == CENTER ? x + w / 2 : x + w, y + (h - tft.fontHeight()) / 2);
        tft.unloadFont();
        tft.setTextDatum(TL_DATUM);
        return;
    }
    spr.fillSprite(bg);
    spr.loadFont(font);
    spr.setTextColor(fg, bg);
    spr.setTextDatum(align == LEFT ? TL_DATUM : align == CENTER ? TC_DATUM : TR_DATUM);
    int ax = align == LEFT ? 0 : align == CENTER ? w / 2 : w;
    spr.drawString(text, ax, (h - spr.fontHeight()) / 2);
    spr.unloadFont();
    spr.pushSprite(x, y);
    spr.deleteSprite();
}

void Field::show(TFT_eSPI& tft, const uint8_t* font, const String& t,
                 int x, int y, int w, int h, Align align, uint16_t fgc, uint16_t bgc) {
    if (t == text && fgc == fg && bgc == bg) return; // Nic się nie zmieniło - nie rysujemy
    text = t; fg = fgc; bg = bgc;
    draw(tft, font, t, x, y, w, h, align, fgc, bgc);
}

} // namespace SmoothText
