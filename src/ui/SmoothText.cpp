#include "SmoothText.h"
#include "Fonts.h"

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

static const uint8_t* fontFor(uint8_t size) {
    return size <= 1 ? FontLabel : (size == 2 ? FontText : FontTitle);
}

int height(TFT_eSPI& tft, uint8_t size) {
    tft.loadFont(fontFor(size));
    int h = tft.fontHeight();
    tft.unloadFont();
    return h;
}

void str(TFT_eSPI& tft, const String& text, int x, int y, uint8_t size, Align align) {
    const uint8_t* font = fontFor(size);
    uint16_t fg  = tft.textcolor, bg = tft.textbgcolor;
    uint16_t pad = tft.getTextPadding();

    if (pad > 0) {
        // Z tłem do wymazania starej wartości: całe pole przez sprite (bez migania)
        tft.loadFont(font);
        int tw = tft.textWidth(text), fh = tft.fontHeight();
        tft.unloadFont();
        int w  = max((int)pad, tw);
        int bx = align == LEFT ? x : (align == CENTER ? x - w / 2 : x - w);
        draw(tft, font, text, bx, y, w, fh, align, fg, bg);
        return;
    }
    // Bez tła: prosto na ekran (napisy statyczne). Kolor tła musi być podany - wyświetlacz nie umie odczytać pikseli.
    tft.loadFont(font);
    tft.setTextColor(fg, bg);
    tft.setTextDatum(align == LEFT ? TL_DATUM : (align == CENTER ? TC_DATUM : TR_DATUM));
    tft.drawString(text, x, y);
    tft.setTextDatum(TL_DATUM);
    tft.unloadFont();
}

void Field::show(TFT_eSPI& tft, const uint8_t* font, const String& t,
                 int x, int y, int w, int h, Align align, uint16_t fgc, uint16_t bgc) {
    if (t == text && fgc == fg && bgc == bg) return; // Nic się nie zmieniło - nie rysujemy
    text = t; fg = fgc; bg = bgc;
    draw(tft, font, t, x, y, w, h, align, fgc, bgc);
}

} // namespace SmoothText
