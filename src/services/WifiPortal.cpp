#include "WifiPortal.h"
#include "Config.h"
#include "SettingsScreen.h"
#include "TripsScreen.h"
#include <WiFi.h>
#include <SD_MMC.h>

// ---------------------------------------------------------------------------
// Strona WWW (jeden plik HTML + JavaScript). Dane pobiera z /api/status i /api/trips.
// ---------------------------------------------------------------------------
static const char PAGE_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="pl"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MotoDash</title>
<style>
 body{margin:0;font-family:system-ui,sans-serif;background:#0b0d10;color:#e8e8e8}
 header{padding:14px 16px;background:#141a22;font-weight:700;color:#3cf}
 nav{display:flex;background:#141a22;border-bottom:1px solid #2a3240}
 nav button{flex:1;padding:12px;background:none;border:0;color:#9aa;font-size:15px}
 nav button.on{color:#fff;border-bottom:3px solid #3cf}
 main{padding:12px;max-width:640px;margin:auto}
 .grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
 .card{background:#141a22;border:1px solid #2a3240;border-radius:10px;padding:12px}
 .lbl{font-size:12px;color:#8c99a6;text-transform:uppercase}
 .val{font-size:26px;font-weight:700;margin-top:4px}
 .unit{font-size:14px;color:#8c99a6;font-weight:400}
 table{width:100%;border-collapse:collapse;font-size:14px}
 th,td{padding:8px 6px;border-bottom:1px solid #2a3240;text-align:right}
 th:first-child,td:first-child{text-align:left}
 label{display:block;margin:12px 0 4px;color:#8c99a6;font-size:13px}
 input{width:100%;box-sizing:border-box;padding:10px;font-size:16px;background:#0b0d10;color:#fff;border:1px solid #2a3240;border-radius:8px}
 .btn{display:inline-block;margin-top:16px;padding:12px 18px;background:#1b6ad8;color:#fff;border:0;border-radius:8px;font-size:16px;text-decoration:none}
 .msg{margin-top:10px;color:#3c3}
 .hide{display:none}
 .red{color:#f44}.yel{color:#fe4}.grn{color:#3e5}.blu{color:#3cf}
</style></head><body>
<header>MotoDash</header>
<nav><button id="b0" class="on" onclick="tab(0)">Stan</button><button id="b1" onclick="tab(1)">Trasy</button><button id="b2" onclick="tab(2)">Ustawienia</button></nav>
<main>
<section id="t0"><div class="grid">
 <div class="card"><div class="lbl">Paliwo w baku</div><div class="val" id="fuel">-</div></div>
 <div class="card"><div class="lbl">Zasieg</div><div class="val blu" id="range">-</div></div>
 <div class="card"><div class="lbl">Srednie spalanie (bak)</div><div class="val grn" id="avg">-</div></div>
 <div class="card"><div class="lbl">Wycieczka</div><div class="val" id="trip">-</div><div class="unit" id="tripAvg"></div></div>
 <div class="card"><div class="lbl">Temp. silnika</div><div class="val" id="temp">-</div></div>
 <div class="card"><div class="lbl">Akumulator</div><div class="val" id="volt">-</div></div>
 <div class="card"><div class="lbl">Otoczenie</div><div class="val" id="amb">-</div></div>
 <div class="card"><div class="lbl">Mnoznik kalibracji</div><div class="val" id="cal">-</div></div>
 <div class="card"><div class="lbl">Opona przod</div><div class="val" id="tf">-</div></div>
 <div class="card"><div class="lbl">Opona tyl</div><div class="val" id="tr">-</div></div>
</div></section>
<section id="t1" class="hide"><div class="card">
 <table><thead><tr><th>#</th><th>Czas</th><th>km</th><th>L/100</th><th>km/h</th></tr></thead><tbody id="trips"></tbody></table>
 <a class="btn" href="/trasy.csv">Pobierz CSV</a>
</div></section>
<section id="t2" class="hide"><div class="card"><form id="f" onsubmit="save();return false">
 <label>Mnoznik kalibracji spalania (0.70 - 1.30)</label><input name="cal" type="number" step="0.01" min="0.7" max="1.3">
 <label>Pojemnosc baku [L] (10 - 30)</label><input name="tank" type="number" step="0.5" min="10" max="30">
 <label>Alarm cisnienia - przod [bar] (1.5 - 2.8)</label><input name="minF" type="number" step="0.1" min="1.5" max="2.8">
 <label>Alarm cisnienia - tyl [bar] (1.5 - 3.2)</label><input name="minR" type="number" step="0.1" min="1.5" max="3.2">
 <label>Wysokosc startowa [m n.p.m.] (0 - 2500)</label><input name="alt" type="number" step="10" min="0" max="2500">
 <button class="btn" type="submit">Zapisz</button><div class="msg" id="msg"></div>
</form></div></section>
</main>
<script>
let filled=false;
function $(i){return document.getElementById(i)}
function tab(n){for(let i=0;i<3;i++){$('t'+i).classList.toggle('hide',i!=n);$('b'+i).classList.toggle('on',i==n)}if(n==1)trips()}
function u(v,un){return v+' <span class="unit">'+un+'</span>'}
async function status(){try{const d=await (await fetch('/api/status')).json();
 const r=d.fuel/d.cap;$('fuel').innerHTML=u(d.fuel.toFixed(1),'/ '+d.cap.toFixed(1)+' L');$('fuel').className='val '+(r<0.1?'red':r<0.25?'yel':'');
 $('range').innerHTML=u(d.range,'km');$('avg').innerHTML=u(d.avg.toFixed(1),'L/100');
 $('trip').innerHTML=u(d.tripKm.toFixed(1),'km');$('tripAvg').textContent='srednio '+d.tripAvg.toFixed(1)+' L/100';
 $('temp').innerHTML=u(d.temp,'&deg;C');$('temp').className='val '+(d.temp>d.tempWarn?'red':'');
 $('volt').innerHTML=u(d.volt.toFixed(1),'V');$('amb').innerHTML=u(d.amb.toFixed(1),'&deg;C');$('amb').className='val '+(d.amb<3?'blu':'');
 $('cal').textContent=d.cal.toFixed(2);
 $('tf').innerHTML=u(d.fBar.toFixed(1),'bar, '+d.fTemp+'&deg;C');$('tf').className='val '+(d.fBar<d.minF?'red':'grn');
 $('tr').innerHTML=u(d.rBar.toFixed(1),'bar, '+d.rTemp+'&deg;C');$('tr').className='val '+(d.rBar<d.minR?'red':'grn');
 if(!filled){const f=$('f');f.cal.value=d.cal.toFixed(2);f.tank.value=d.cap;f.minF.value=d.minF.toFixed(1);f.minR.value=d.minR.toFixed(1);f.alt.value=d.alt;filled=true}
}catch(e){}}
async function trips(){try{const d=await (await fetch('/api/trips')).json();
 $('trips').innerHTML=d.length?d.map(t=>'<tr><td>'+t.id+'</td><td>'+String(Math.floor(t.min/60)).padStart(2,'0')+':'+String(t.min%60).padStart(2,'0')+'</td><td>'+t.km.toFixed(1)+'</td><td>'+t.fuel.toFixed(1)+'</td><td>'+t.spd+'</td></tr>').join(''):'<tr><td colspan="5">Brak zapisanych tras</td></tr>';
}catch(e){}}
async function save(){const b=new URLSearchParams(new FormData($('f')));
 try{const r=await fetch('/api/settings',{method:'POST',body:b});$('msg').textContent=r.ok?'Zapisano w MotoDash':'Blad zapisu';filled=false;status()}catch(e){$('msg').textContent='Brak polaczenia'}}
status();setInterval(status,2000);
</script></body></html>)HTML";

// ---------------------------------------------------------------------------
// Start / stop
// ---------------------------------------------------------------------------

void WifiPortal::init(MotoState* statePtr, SettingsScreen* settingsPtr, TripsScreen* tripsPtr) {
    state    = statePtr;
    settings = settingsPtr;
    trips    = tripsPtr;

    // Trasy rejestrujemy raz - serwer startuje dopiero po włączeniu hotspotu
    server.on("/",             HTTP_GET,  [this]() { handleRoot(); });
    server.on("/api/status",   HTTP_GET,  [this]() { handleStatus(); });
    server.on("/api/trips",    HTTP_GET,  [this]() { handleTrips(); });
    server.on("/trasy.csv",    HTTP_GET,  [this]() { handleCsv(); });
    server.on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });
    server.onNotFound(                    [this]() { handleNotFound(); });

    WiFi.mode(WIFI_OFF); // Radio wyłączone, dopóki użytkownik nie włączy hotspotu
}

void WifiPortal::start() {
    if (active) return;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(Config::WIFI_AP_SSID, Config::WIFI_AP_PASS);
    IPAddress ip = WiFi.softAPIP();

    dns.start(53, "*", ip); // Każda nazwa -> MotoDash: telefon sam zaproponuje otwarcie strony
    server.begin();
    active          = true;
    stoppedByRiding = false;
    Serial.println("[WifiPortal] Hotspot " + String(Config::WIFI_AP_SSID) + " wlaczony: " + address());
}

void WifiPortal::stop() {
    if (!active) return;
    server.stop();
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    active = false;
    Serial.println("[WifiPortal] Hotspot wylaczony");
}

void WifiPortal::update() {
    if (!active) return;

    // Hotspot nie jest potrzebny w trasie - wyłączamy go po ruszeniu (jeśli włączone w Config.h)
    if (Config::WIFI_OFF_WHEN_RIDING && state && state->speedKmh > Config::WIFI_OFF_SPEED_KMH) {
        stop();
        stoppedByRiding = true;
        Serial.println("[WifiPortal] Wylaczono hotspot - motocykl w ruchu");
        return;
    }

    dns.processNextRequest();
    server.handleClient();
}

int WifiPortal::clientCount() const {
    return active ? WiFi.softAPgetStationNum() : 0;
}

String WifiPortal::address() const {
    return "http://" + WiFi.softAPIP().toString();
}

// ---------------------------------------------------------------------------
// Obsługa żądań
// ---------------------------------------------------------------------------

void WifiPortal::handleRoot() {
    server.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}

void WifiPortal::handleStatus() {
    const MotoState& s = *state;
    String j;
    j.reserve(420);
    j += "{\"fuel\":"   + String(s.fuelInTankL, 2);
    j += ",\"cap\":"    + String(s.tankCapacityL, 1);
    j += ",\"range\":"  + String(s.rangeKm);
    j += ",\"avg\":"    + String(s.tankAvgFuel100, 2);
    j += ",\"tripKm\":" + String(s.tripKm, 2);
    j += ",\"tripAvg\":"+ String(s.tripAvgFuel100, 2);
    j += ",\"temp\":"   + String(s.engineTempC);
    j += ",\"tempWarn\":" + String(Config::ENGINE_TEMP_WARN);
    j += ",\"volt\":"   + String(s.voltageV, 2);
    j += ",\"amb\":"    + String(s.ambientTempC, 1);
    j += ",\"fBar\":"   + String(s.frontBar, 2);
    j += ",\"fTemp\":"  + String(s.frontTempC);
    j += ",\"rBar\":"   + String(s.rearBar, 2);
    j += ",\"rTemp\":"  + String(s.rearTempC);
    j += ",\"minF\":"   + String(s.minFrontBar, 2);
    j += ",\"minR\":"   + String(s.minRearBar, 2);
    j += ",\"cal\":"    + String(s.fuelCalFactor, 3);
    j += ",\"alt\":"    + String(s.baseAltitudeM);
    j += "}";
    server.send(200, "application/json", j);
}

void WifiPortal::handleTrips() {
    const MotoState& s = *state;
    String j = "[";
    for (int i = 0; i < s.historyCount; i++) {
        const TripRecord& t = s.history[i];
        if (i) j += ",";
        j += "{\"id\":" + String(t.id) + ",\"min\":" + String(t.durationMin) +
             ",\"km\":" + String(t.distanceKm, 1) + ",\"fuel\":" + String(t.avgFuel100, 1) +
             ",\"spd\":" + String(t.avgSpeedKmh) + "}";
    }
    j += "]";
    server.send(200, "application/json", j);
}

// Pełna historia z karty SD; bez karty - trasy z pamięci (tabela z ekranu)
void WifiPortal::handleCsv() {
    server.sendHeader("Content-Disposition", "attachment; filename=historia_tras.csv");
    if (trips && trips->hasSd() && SD_MMC.exists("/historia_tras.csv")) {
        fs::File f = SD_MMC.open("/historia_tras.csv", FILE_READ);
        if (f) {
            server.streamFile(f, "text/csv");
            f.close();
            return;
        }
    }
    const MotoState& s = *state;
    String csv = "Nr_Trasy;Czas_min;Dystans_km;Srednie_spalanie_L100;Srednia_predkosc_kmh\n";
    for (int i = s.historyCount - 1; i >= 0; i--) { // Od najstarszej, jak w pliku na karcie
        const TripRecord& t = s.history[i];
        csv += String(t.id) + ";" + String(t.durationMin) + ";" + String(t.distanceKm, 1) + ";" +
               String(t.avgFuel100, 1) + ";" + String(t.avgSpeedKmh) + "\n";
    }
    server.send(200, "text/csv", csv);
}

// Zapis ustawień z formularza - te same zakresy co w menu Konfiguracji na wyświetlaczu
void WifiPortal::handleSaveSettings() {
    MotoState& s = *state;
    if (server.hasArg("cal"))  s.fuelCalFactor = constrain(server.arg("cal").toFloat(),  Config::CAL_FACTOR_MIN, Config::CAL_FACTOR_MAX);
    if (server.hasArg("tank")) s.tankCapacityL = constrain(server.arg("tank").toFloat(), 10.0f, 30.0f);
    if (server.hasArg("minF")) s.minFrontBar   = constrain(server.arg("minF").toFloat(), 1.5f, 2.8f);
    if (server.hasArg("minR")) s.minRearBar    = constrain(server.arg("minR").toFloat(), 1.5f, 3.2f);
    if (server.hasArg("alt"))  s.baseAltitudeM = constrain(server.arg("alt").toInt(),    0, 2500);

    if (settings) settings->saveToNvs(s);
    s.forceRedraw = true; // Wyświetlacz pokaże nowe wartości
    Serial.println("[WifiPortal] Zapisano ustawienia ze strony WWW");
    server.send(200, "application/json", "{\"ok\":true}");
}

// Każdy inny adres (np. sprawdzanie internetu przez telefon) -> przekierowanie na stronę MotoDash
void WifiPortal::handleNotFound() {
    server.sendHeader("Location", address() + "/", true);
    server.send(302, "text/plain", "");
}
