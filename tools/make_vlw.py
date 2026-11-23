#!/usr/bin/env python3
"""Generator gładkich czcionek TFT_eSPI (.vlw) z plików TTF/OTF -> nagłówki C (tablice PROGMEM).

Format VLW (big-endian int32): nagłówek 6x int32 (liczba znaków, wersja, rozmiar, 0, ascent, descent),
potem 7x int32 na znak (unicode, wysokość, szerokość, xAdvance, dY, dX, 0), potem bitmapy (1 bajt = krycie).
Użycie: python3 tools/make_vlw.py  (generuje src/ui/fonts/*.h)
"""
import struct, sys, os
from PIL import ImageFont

def make_vlw(path, size, chars):
    font = ImageFont.truetype(path, size)
    asc, desc = font.getmetrics()
    chars = sorted(set(chars), key=ord)
    glyphs, bitmaps = [], []
    for ch in chars:
        adv = int(round(font.getlength(ch)))
        if ch == ' ':
            glyphs.append((ord(ch), 0, 0, adv, 0, 0)); bitmaps.append(b''); continue
        mask, (ox, oy) = font.getmask2(ch, mode='L', anchor='ls')
        w, h = mask.size
        data = bytes(mask)
        glyphs.append((ord(ch), h, w, adv, -oy, ox)); bitmaps.append(data)
    # Wysokość wiersza z faktycznych znaków (nagłówek fontu ma zapas na akcenty -> puste miejsce nad cyframi)
    asc  = max(g[4] for g in glyphs if g[1])
    desc = max(max(g[1] - g[4] for g in glyphs if g[1]), 0)
    out = struct.pack('>6i', len(glyphs), 11, size, 0, asc, desc)
    for g in glyphs: out += struct.pack('>7i', *g, 0)
    for b in bitmaps: out += b
    return out

def to_header(name, data, comment):
    lines = [f"// {comment}", "// Wygenerowano narzedziem tools/make_vlw.py - nie edytowac recznie.",
             "#pragma once", "#include <pgmspace.h>", "", f"const uint8_t {name}[] PROGMEM = {{"]
    for i in range(0, len(data), 24):
        lines.append("  " + ",".join(f"0x{b:02X}" for b in data[i:i+24]) + ",")
    lines.append("};"); lines.append("")
    return "\n".join(lines)

NUM = "0123456789.-° "
TXT = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-/:%°()"
# Katalog z plikami czcionki Inter (https://rsms.me/inter/ lub Google Fonts, licencja OFL).
# Można podać inny: MOTODASH_FONTS=/sciezka python3 tools/make_vlw.py
INTER = os.environ.get("MOTODASH_FONTS", "/usr/share/fonts/opentype/inter").rstrip("/") + "/"
FONTS = [  # nazwa, plik, rozmiar, znaki, opis
    ("FontNumXL", INTER + "InterDisplay-Bold.otf",     58, NUM, "Inter Display Bold 58 px - duze liczby (temperatura, spalanie)"),
    ("FontNumL",  INTER + "InterDisplay-Bold.otf",     30, NUM, "Inter Display Bold 30 px - srednie liczby (napiecie, paliwo, zasieg)"),
    ("FontNumM",  INTER + "InterDisplay-SemiBold.otf", 20, NUM, "Inter Display SemiBold 20 px - male liczby (opony, srednie spalanie)"),
    ("FontLabel", INTER + "Inter-SemiBold.otf",        12, TXT, "Inter SemiBold 12 px - podpisy"),
]
if __name__ == "__main__":
    outdir = sys.argv[1] if len(sys.argv) > 1 else "src/ui/fonts"
    os.makedirs(outdir, exist_ok=True)
    for name, path, size, chars, desc in FONTS:
        data = make_vlw(path, size, chars)
        open(os.path.join(outdir, name + ".h"), "w").write(to_header(name, data, desc))
        print(f"{name}: {len(data)} B")
