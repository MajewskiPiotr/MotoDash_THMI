#include "TripsScreen.h"
#include "Theme.h" // Wspólna paleta kolorów
#include "Config.h"
#include <SPI.h>
#include <SD.h> // Wbudowana biblioteka ESP32 do obsługi karty pamięci microSD


// Piny czytnika kart microSD na płytce LilyGO T-HMI
static constexpr uint8_t SD_CS   = 51;
static constexpr uint8_t SD_MISO = 50;
static constexpr uint8_t SD_MOSI = 49;
static constexpr uint8_t SD_SCLK = 52;

// Inicjalizuje kartę microSD przy starcie systemu lub ładuje dane przykładowe do podglądu
void TripsScreen::initSdAndLoad(MotoState& state) {
    // Tworzymy przykładowe 3 trasy na start, aby tabela od razu była czytelna podczas testów
    state.history[0] = { 3, 145, 168.4f, 4.9f, 70 };
    state.history[1] = { 2,  82,  94.2f, 5.2f, 68 };
    state.history[2] = { 1, 210, 245.0f, 4.7f, 71 };
    state.historyCount = 3;
    state.nextTripId   = 4;

    // Próba uruchomienia karty microSD (jeśli jest włożona w slot T-HMI)
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    if (SD.begin(SD_CS)) {
        sdAvailable = true;
        Serial.println("[TripsScreen] Karta microSD zamontowana poprawnie!");

        // Jeśli plik CSV jeszcze nie istnieje, tworzymy go z wierszem nagłówkowym dla Excela
        if (!SD.exists("/historia_tras.csv")) {
            File file = SD.open("/historia_tras.csv", FILE_WRITE);
            if (file) {
                file.println("Nr_Trasy;Czas_min;Dystans_km;Srednie_spalanie_L100;Srednia_predkosc_kmh");
                file.close();
            }
        }
    } else {
        sdAvailable = false;
        Serial.println("[TripsScreen] Brak karty microSD - historia dziala w pamieci RAM");
    }
}

// Dodaje nową zakończoną trasę na samą górę tabeli (przesuwając starsze w dół) i zapisuje na SD
void TripsScreen::archiveCurrentTrip(MotoState& state, int durationMin, float distKm, float avgFuel, int avgSpeed) {
    // Przesuwamy istniejące wpisy o 1 pozycję w dół (najstarszy 5. wpis wypada z widoku)
    for (int i = MotoState::MAX_HISTORY - 1; i > 0; i--) {
        state.history[i] = state.history[i - 1];
    }

    // Wpisujemy najnowszą trasę pod indeks 0 (pierwszy wiersz od góry)
    state.history[0] = { state.nextTripId++, durationMin, distKm, avgFuel, avgSpeed };
    if (state.historyCount < MotoState::MAX_HISTORY) {
        state.historyCount++;
    }

    // Dopisujemy rekord do pliku CSV na karcie SD (jeśli karta jest obecna)
    if (sdAvailable) {
        saveToSdCsv(state.history[0]);
    }

    isDirty = true; // Wymusza przerysowanie tabeli na ekranie
}

// Dopisuje pojedynczy wiersz danych w formacie CSV na kartę microSD
void TripsScreen::saveToSdCsv(const TripRecord& rec) {
    File file = SD.open("/historia_tras.csv", FILE_APPEND);
    if (file) {
        file.printf("%d;%d;%.1f;%.1f;%d\n",
                    rec.id, rec.durationMin, rec.distanceKm, rec.avgFuel100, rec.avgSpeedKmh);
        file.close();
        Serial.println("[TripsScreen] Zapisano trase #" + String(rec.id) + " na karte SD");
    }
}

// Zamienia łączną liczbę minut (np. 145) na czytelny format godzinowy ("02:25")
String TripsScreen::formatDuration(int totalMinutes) {
    int h = totalMinutes / 60;
    int m = totalMinutes % 60;
    char buf[10];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    return String(buf);
}

// Rysuje statyczny szkielet tabeli (nagłówek ekranu, belkę tytułów kolumn i legendę na dole)
void TripsScreen::onEnter(TFT_eSPI& tft, const MotoState& state) {
    isDirty = true; // Wymusza narysowanie wierszy danych przy wejściu na ekran
    tft.fillScreen(BG_COLOR);

    // Tytuł ekranu + status karty SD w prawym górnym rogu (przed kropkami stron)
    tft.setTextColor(ACCENT_BLUE, BG_COLOR);
    tft.drawString("OSTATNIE 5 TRAS", 10, 6, 1);

    tft.setTextColor(sdAvailable ? ACCENT_GREEN : ACCENT_YELLOW, BG_COLOR);
    tft.drawString(sdAvailable ? "[SD OK]" : "[BRAK SD]", 125, 6, 1);

    // --- NAGŁÓWEK KOLUMN TABELI (Y: 22..44) ---
    tft.fillRoundRect(8, 22, 304, 22, 4, HEADER_BG);
    tft.setTextColor(TFT_WHITE, HEADER_BG);
    tft.drawString("#",       14, 29, 1); // Numer trasy
    tft.drawString("CZAS",    42, 29, 1); // Czas jazdy [hh:mm]
    tft.drawString("DYSTANS", 102, 29, 1);// Przejechane kilometry [km]
    tft.drawString("SPAL.",   178, 29, 1);// Średnie spalanie [L/100]
    tft.drawString("SR.V",    246, 29, 1);// Średnia prędkość [km/h]

    // Obramowanie całej tabeli (Y: 22..206)
    tft.drawRoundRect(8, 22, 304, 184, 6, BORDER_COLOR);

    // Dolna legenda przycisków
    tft.setTextColor(TEXT_MUTED, BG_COLOR);
    tft.drawString("[MODE 2s]: Zapisz trase", 10, 218, 1);
    tft.drawString("[ACT 2s]: Wyczysc", 195, 218, 1);
}

// Rysuje 5 wierszy tabeli z danymi ostatnich tras (tylko gdy zaszła zmiana - zero migotania)
void TripsScreen::update(TFT_eSPI& tft, const MotoState& state) {
    if (!isDirty) return;
    isDirty = false;

    // Czyścimy obszar wierszy wewnątrz ramki tabeli
    tft.fillRect(10, 46, 300, 158, BG_COLOR);

    if (state.historyCount == 0) {
        tft.setTextColor(TEXT_MUTED, BG_COLOR);
        tft.drawCentreString("Brak zapisanych tras", 160, 115, 2);
        return;
    }

    // Rysujemy po kolei do 5 ostatnich tras
    for (int i = 0; i < state.historyCount; i++) {
        int y = 47 + (i * 31); // Każdy wiersz ma 31 px wysokości
        uint16_t rowBg = (i % 2 == 0) ? CARD_COLOR : ROW_ALT_COLOR;

        // Tło pojedynczego wiersza
        tft.fillRoundRect(11, y, 298, 28, 3, rowBg);

        const TripRecord& tr = state.history[i];

        // 1. Numer trasy (#)
        tft.setTextColor(ACCENT_BLUE, rowBg);
        tft.drawString(String(tr.id), 15, y + 7, 2);

        // 2. Czas jazdy (hh:mm)
        tft.setTextColor(TFT_WHITE, rowBg);
        tft.drawString(formatDuration(tr.durationMin), 40, y + 7, 2);

        // 3. Dystans (km)
        tft.setTextColor(ACCENT_GREEN, rowBg);
        tft.drawString(String(tr.distanceKm, 1) + "k", 102, y + 7, 2);

        // 4. Średnie spalanie (L/100km)
        tft.setTextColor(ACCENT_YELLOW, rowBg);
        tft.drawString(String(tr.avgFuel100, 1) + "L", 178, y + 7, 2);

        // 5. Średnia prędkość (km/h)
        tft.setTextColor(TFT_WHITE, rowBg);
        tft.drawString(String(tr.avgSpeedKmh) + "km/h", 242, y + 7, 2);
    }
}

// Długie przytrzymanie MODE archiwizuje bieżącą trasę do tabeli i na kartę SD
bool TripsScreen::onModeLongPress(MotoState& state) {
    // Wyliczamy przykładowy czas jazdy (np. na podstawie dystansu i średniej prędkości 65 km/h)
    float dist = (state.tripKm > 0.5f) ? state.tripKm : 42.5f; // Jeśli 0 km w teście, wpisuje próbkę 42.5 km
    int avgSpd = 65;
    int durMin = (int)((dist / avgSpd) * 60.0f);

    archiveCurrentTrip(state, durMin, dist, state.tripAvgFuel100 , avgSpd);
    return true;
}

// Długie przytrzymanie ACTION czyści podgląd tabeli na ekranie
bool TripsScreen::onActionLongPress(MotoState& state) {
    state.historyCount = 0;
    isDirty = true;
    Serial.println("[TripsScreen] Wyczyszczono tabele historii");
    return true;
}
