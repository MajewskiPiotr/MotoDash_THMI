#pragma once
#include <stdint.h>

// Wspólna paleta kolorów wszystkich ekranów (format RGB565).
// Zmiana koloru tutaj zmienia go w całej aplikacji.

// --- TŁA I RAMKI ---
constexpr uint16_t BG_COLOR      = 0x0841; // Ciemny grafit (tło główne)
constexpr uint16_t CARD_COLOR    = 0x10A3; // Ciemnoszary (wnętrze kafelków)
constexpr uint16_t ROW_ALT_COLOR = 0x18E4; // Nieco jaśniejszy szary (co drugi wiersz tabeli)
constexpr uint16_t HIGHLIGHT_BG  = 0x1B4F; // Granatowy (zaznaczona opcja / wiersz)
constexpr uint16_t HEADER_BG     = 0x1B4F; // Granatowy (nagłówek tabeli)
constexpr uint16_t BORDER_COLOR  = 0x2965; // Szary (obramowania kafelków)

// --- TEKST ---
constexpr uint16_t TEXT_MUTED    = 0x8C71; // Przygaszony jasnoszary (etykiety, podpowiedzi)

// --- AKCENTY ---
constexpr uint16_t ACCENT_GREEN  = 0x07E0; // Zielony (wartości prawidłowe, "DO PELNA")
constexpr uint16_t ACCENT_BLUE   = 0x05FF; // Błękitny (nagłówki, zasięg, ostrzeżenie o gołoledzi)
constexpr uint16_t ACCENT_YELLOW = 0xFFE0; // Żółty (niski poziom paliwa, ostrzeżenia)
constexpr uint16_t ACCENT_RED    = 0xF800; // Czerwony (alarmy)
constexpr uint16_t BAR_PREVIEW   = 0x0320; // Ciemnozielony (podgląd stanu baku po dolaniu)
constexpr uint16_t DOT_INACTIVE  = 0x4A69; // Szary (nieaktywna kropka wskaźnika stron)
