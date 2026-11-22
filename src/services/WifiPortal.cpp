#include "WifiPortal.h"
#include "Config.h"
#include "SettingsScreen.h"
#include "TripsScreen.h"
#include "RefuelLog.h"
#include "FuelCalculator.h"
#include <sys/time.h>
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
 .tiles{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
 .sub{font-size:12px;color:#8c99a6;margin-top:4px}
 h2{font-size:15px;margin:18px 0 8px;color:#c3c2b7;font-weight:600}
 .legend{display:flex;gap:14px;font-size:13px;color:#c3c2b7;margin-bottom:6px;flex-wrap:wrap}
 .sw{display:inline-block;width:12px;height:12px;border-radius:3px;margin-right:6px;vertical-align:-1px}
 .scroll{overflow-x:auto}
 #rt th,#rt td{padding:7px 5px;white-space:nowrap;font-size:12.5px}
 #rt th:nth-child(2),#rt td:nth-child(2){text-align:left}
 #rt tr.tank td{border-top:1px solid #2a3240;border-bottom:0;font-weight:600}
 #rt tr.ev td{color:#c3c2b7;font-size:12px;border-bottom:0;padding-top:3px;padding-bottom:3px}
 #rt tr.ev td:first-child{padding-left:14px}
 .tag{display:inline-block;padding:1px 6px;border-radius:5px;font-size:11px;font-weight:600}
 .t-full{background:#12324a;color:#8fd0ff}.t-top{background:#3a2a12;color:#ffc477}
 .t-skip{background:#3a1a1a;color:#ff9a9a}.t-open{background:#22302a;color:#8fe0b0}.t-nm{background:#2a2a2a;color:#bbb}
 .muted{color:#8c99a6}.note{font-size:12px;color:#8c99a6;margin-top:10px;line-height:1.5}
 .tip{position:fixed;pointer-events:none;background:#0b0d10;border:1px solid #2a3240;border-radius:8px;padding:8px 10px;font-size:13px;display:none;z-index:5}
</style></head><body>
<header>MotoDash</header>
<nav><button id="b0" class="on" onclick="tab(0)">Stan</button><button id="b1" onclick="tab(1)">Trasy</button><button id="b3" onclick="tab(3)">Tankowania</button><button id="b2" onclick="tab(2)">Ustawienia</button></nav>
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
<section id="t3" class="hide">
 <div class="tiles">
  <div class="card"><div class="lbl">Mnoznik kalibracji</div><div class="val" id="rf">-</div><div class="sub" id="rf0"></div></div>
  <div class="card"><div class="lbl">Spalanie z dystrybutora</div><div class="val" id="rreal">-</div><div class="sub" id="rreal0">srednia z 3 ostatnich bakow</div></div>
  <div class="card"><div class="lbl">Biezacy okres</div><div class="val" id="ropen">-</div><div class="sub" id="ropen0"></div></div>
 </div>
 <h2>Szacunek komputera a spalanie z przeliczenia</h2>
 <div class="card"><div class="legend"><span><span class="sw" style="background:#3987e5"></span>Szacunek komputera</span><span><span class="sw" style="background:#d95926"></span>Z przeliczenia (dystrybutor)</span><span class="muted">przygaszone = bak pominiety</span></div>
 <svg id="rc" viewBox="0 0 700 262" width="100%" role="img" aria-label="Spalanie: szacunek a przeliczenie"></svg><div class="muted" id="rcEmpty"></div></div>
 <h2>Baki i tankowania</h2>
 <div class="card scroll"><table id="rt"><thead><tr><th>Bak / tankowanie</th><th>Okres</th><th>km</th><th>Litry</th><th>Szacunek<br>L/100</th><th>Przeliczenie<br>L/100</th><th>Roznica</th><th>Mnoznik</th></tr></thead><tbody id="rtb"></tbody></table>
 <a class="btn" href="/tankowania.csv">Pobierz CSV</a>
 <div class="note">Przeliczenie = (litry przy "do pelna" + dolewki z okresu) / km okresu x 100. Mnoznik: srednia wazona litrami z 3 ostatnich waznych bakow; bak z roznica powyzej 30% jest pomijany.</div></div>
</section>
<section id="t2" class="hide"><div class="card"><form id="f" onsubmit="save();return false">
 <label>Mnoznik kalibracji spalania (0.70 - 1.30)</label><input name="cal" type="number" step="0.01" min="0.7" max="1.3">
 <label>Pojemnosc baku [L] (10 - 30)</label><input name="tank" type="number" step="0.5" min="10" max="30">
 <label>Alarm cisnienia - przod [bar] (1.5 - 2.8)</label><input name="minF" type="number" step="0.1" min="1.5" max="2.8">
 <label>Alarm cisnienia - tyl [bar] (1.5 - 3.2)</label><input name="minR" type="number" step="0.1" min="1.5" max="3.2">
 <label>Wysokosc startowa [m n.p.m.] (0 - 2500)</label><input name="alt" type="number" step="10" min="0" max="2500">
 <button class="btn" type="submit">Zapisz</button><div class="msg" id="msg"></div>
</form></div></section>
</main>
<div class="tip" id="tip"></div>
<script>
let filled=false;
function $(i){return document.getElementById(i)}
function tab(n){for(let i=0;i<4;i++){$('t'+i).classList.toggle('hide',i!=n);$('b'+i).classList.toggle('on',i==n)}if(n==1)trips();if(n==3)refuels()}
function u(v,un){return v+' <span class="unit">'+un+'</span>'}
async function status(){try{const d=await (await fetch('/api/status')).json();
 const r=d.fuel/d.cap;$('fuel').innerHTML=u(d.fuel.toFixed(1),'/ '+d.cap.toFixed(1)+' L');$('fuel').className='val '+(r<0.1?'red':r<0.25?'yel':'');
 $('range').innerHTML=u(d.range,'km');$('avg').innerHTML=u(d.avg.toFixed(1),'L/100');
 $('trip').innerHTML=u(d.tripKm.toFixed(1),'km');$('tripAvg').textContent='srednio '+d.tripAvg.toFixed(1)+' L/100';
 $('temp').innerHTML=u(d.temp,'&deg;C');$('temp').className='val '+(d.temp>d.tempWarn?'red':'');
 $('volt').innerHTML=u(d.volt.toFixed(1),'V');$('amb').innerHTML=u(d.amb.toFixed(1),'&deg;C');$('amb').className='val '+(d.amb<3?'blu':'');
 $('cal').textContent=d.cal.toFixed(2);
 const ty=(id,ok,b,t,bt,mn)=>{if(!ok){$(id).innerHTML='&mdash; <span class="unit">brak odczytu</span>';$(id).className='val';return}
  $(id).innerHTML=u(b.toFixed(1),'bar, '+t+'&deg;C'+(bt>=0?', bat. '+bt+'%':''));$(id).className='val '+(b<mn?'red':'grn')};
 ty('tf',d.fOk,d.fBar,d.fTemp,d.fBat,d.minF);ty('tr',d.rOk,d.rBar,d.rTemp,d.rBat,d.minR);
 if(!filled){const f=$('f');f.cal.value=d.cal.toFixed(2);f.tank.value=d.cap;f.minF.value=d.minF.toFixed(1);f.minR.value=d.minR.toFixed(1);f.alt.value=d.alt;filled=true}
}catch(e){}}
async function trips(){try{const d=await (await fetch('/api/trips')).json();
 $('trips').innerHTML=d.length?d.map(t=>'<tr><td>'+t.id+'</td><td>'+String(Math.floor(t.min/60)).padStart(2,'0')+':'+String(t.min%60).padStart(2,'0')+'</td><td>'+t.km.toFixed(1)+'</td><td>'+t.fuel.toFixed(1)+'</td><td>'+t.spd+'</td></tr>').join(''):'<tr><td colspan="5">Brak zapisanych tras</td></tr>';
}catch(e){}}
async function save(){const b=new URLSearchParams(new FormData($('f')));
 try{const r=await fetch('/api/settings',{method:'POST',body:b});$('msg').textContent=r.ok?'Zapisano w MotoDash':'Blad zapisu';filled=false;status()}catch(e){$('msg').textContent='Brak polaczenia'}}
const f1=(v,d=1)=>v.toFixed(d).replace('.',',');
function dt(t,full){if(!t)return '\u2014';const d=new Date(t*1000),p=x=>String(x).padStart(2,'0');return p(d.getDate())+'.'+p(d.getMonth()+1)+(full?' '+p(d.getHours())+':'+p(d.getMinutes()):'')}
async function refuels(){try{const d=await (await fetch('/api/refuels')).json();
 $('rf').textContent=f1(d.factor,2);
 const ok=d.tanks.filter(t=>t.s==0).slice(0,3),sr=ok.reduce((a,t)=>a+t.real,0),sk=ok.reduce((a,t)=>a+t.km,0);
 $('rreal').innerHTML=sk>0?u(f1(sr/sk*100,2),'L/100'):'\u2014';
 if(d.tanks.length){const first=d.tanks[d.tanks.length-1];$('rf0').textContent='byl '+f1(first.f0,2)+' przed bakiem '+first.n}
 const o=d.open;$('ropen').innerHTML=u(Math.round(o.km),'km');
 $('ropen0').textContent='od "do pelna" '+dt(o.st)+', '+(o.tn?('dolewki: '+o.tn+'x, '+f1(o.tl)+' L'):'bez dolewek');
 const tg={0:'<span class="tag t-full">do pelna</span>',1:'<span class="tag t-top">dolewka</span>',2:'<span class="tag t-nm">do pelna, bez litrow</span>'};
 let r='<tr class="tank"><td><span class="tag t-open">trwa</span> bak '+o.n+'</td><td>od '+dt(o.st)+'</td><td>'+Math.round(o.km)+'</td><td class="muted">\u2014</td><td>'+(o.km>0.2?f1(o.est/o.km*100,2):'\u2014')+'</td><td class="muted">\u2014</td><td></td><td>'+f1(d.factor,2)+'</td></tr>';
 for(const e of [...o.ev].reverse())r+='<tr class="ev"><td>'+tg[e.y]+' '+dt(e.t,1)+'</td><td></td><td class="muted">'+Math.round(e.o)+' km</td><td>'+f1(e.l)+' L</td><td></td><td></td><td></td><td></td></tr>';
 for(const t of d.tanks){const meas=t.real>0,diff=meas?(t.real/t.est-1)*100:0,
   st=t.s==1?' <span class="tag t-skip">pominiety</span>':t.s==2?' <span class="tag t-nm">bez pomiaru</span>':'';
  r+='<tr class="tank"><td>Bak '+t.n+st+'</td><td>'+dt(t.st)+' \u2013 '+dt(t.et)+'</td><td>'+Math.round(t.km)+'</td><td>'+(meas?f1(t.real):'\u2014')+'</td><td>'+(t.km>0.2?f1(t.est/t.km*100,2):'\u2014')+'</td><td>'+(meas?f1(t.real/t.km*100,2):'\u2014')+'</td><td>'+(meas?(diff>0?'+':'')+f1(diff)+'%':'')+'</td><td>'+(Math.abs(t.f1-t.f0)<0.005?f1(t.f1,2):f1(t.f0,2)+' \u2192 '+f1(t.f1,2))+'</td></tr>';
  for(const e of [...t.ev].reverse())r+='<tr class="ev"><td>'+tg[e.y]+' '+dt(e.t,1)+'</td><td></td><td class="muted">'+Math.round(e.o)+' km</td><td>'+f1(e.l)+' L</td><td></td><td></td><td></td><td></td></tr>';}
 $('rtb').innerHTML=r;chart(d.tanks.filter(t=>t.real>0&&t.km>0.2).reverse());
}catch(e){}}
function chart(ts){const svg=$('rc'),NS='http://www.w3.org/2000/svg';svg.innerHTML='';$('rcEmpty').textContent=ts.length?'':'Wykres pojawi sie po pierwszym tankowaniu do pelna z podaniem litrow.';
 if(!ts.length)return;const W=700,H=262,L=44,R=10,T=26,B=40,vals=ts.flatMap(t=>[t.est/t.km*100,t.real/t.km*100]),max=Math.max(4,Math.ceil(Math.max(...vals)+0.5));
 const y=v=>T+(H-T-B)*(1-v/max),step=(W-L-R)/ts.length,bw=Math.min(26,step/2-4),gap=2,tip=$('tip');
 const el=(n,a,x)=>{const e=document.createElementNS(NS,n);for(const k in a)e.setAttribute(k,a[k]);if(x!=null)e.textContent=x;svg.appendChild(e);return e};
 for(let v=0;v<=max;v++){el('line',{x1:L,x2:W-R,y1:y(v),y2:y(v),stroke:v?'#2c2c2a':'#383835'});el('text',{x:L-8,y:y(v)+4,'text-anchor':'end','font-size':11,fill:'#8c99a6'},v)}
 el('text',{x:4,y:12,'font-size':11,fill:'#8c99a6'},'L/100 km');
 const bar=(x,v,c,op)=>{const tp=y(v),bt=y(0),r=4;el('path',{d:`M${x},${bt} V${tp+r} Q${x},${tp} ${x+r},${tp} H${x+bw-r} Q${x+bw},${tp} ${x+bw},${tp+r} V${bt} Z`,fill:c,opacity:op})};
 ts.forEach((t,i)=>{const cx=L+step*i+step/2,op=t.s==1?0.35:1,e=t.est/t.km*100,re=t.real/t.km*100,df=(t.real/t.est-1)*100;
  bar(cx-bw-gap/2,e,'#3987e5',op);bar(cx+gap/2,re,'#d95926',op);
  el('text',{x:cx,y:H-B+18,'text-anchor':'middle','font-size':12,fill:'#c3c2b7'},'Bak '+t.n);
  el('text',{x:cx,y:H-B+33,'text-anchor':'middle','font-size':11,fill:'#8c99a6'},t.s==1?'pominiety':(df>0?'+':'')+f1(df)+'%');
  const h=el('rect',{x:cx-step/2+2,y:T,width:step-4,height:H-T-B,fill:'transparent'});
  const show=ev=>{tip.style.display='block';tip.style.left=(ev.clientX+12)+'px';tip.style.top=(ev.clientY+12)+'px';tip.innerHTML='<b>Bak '+t.n+'</b> ('+Math.round(t.km)+' km)<br>Szacunek: '+f1(e,2)+' L/100<br>Przeliczenie: '+f1(re,2)+' L/100'};
  h.addEventListener('mousemove',show);h.addEventListener('touchstart',ev=>show(ev.touches[0]),{passive:true});h.addEventListener('mouseleave',()=>tip.style.display='none')})}
fetch('/api/time',{method:'POST',body:new URLSearchParams({t:Math.floor(Date.now()/1000)})}).catch(()=>{});
status();setInterval(status,2000);
</script></body></html>)HTML";

// ---------------------------------------------------------------------------
// Start / stop
// ---------------------------------------------------------------------------

void WifiPortal::init(MotoState* statePtr, SettingsScreen* settingsPtr, TripsScreen* tripsPtr,
                      RefuelLog* refuelPtr, FuelCalculator* fuelPtr) {
    state    = statePtr;
    settings = settingsPtr;
    trips    = tripsPtr;
    refuels  = refuelPtr;
    fuel     = fuelPtr;

    // Trasy rejestrujemy raz - serwer startuje dopiero po włączeniu hotspotu
    server.on("/",             HTTP_GET,  [this]() { handleRoot(); });
    server.on("/api/status",   HTTP_GET,  [this]() { handleStatus(); });
    server.on("/api/trips",    HTTP_GET,  [this]() { handleTrips(); });
    server.on("/trasy.csv",    HTTP_GET,  [this]() { handleCsv(); });
    server.on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });
    server.on("/api/refuels",  HTTP_GET,  [this]() { handleRefuels(); });
    server.on("/tankowania.csv", HTTP_GET, [this]() { handleRefuelsCsv(); });
    server.on("/api/time",     HTTP_POST, [this]() { handleSetTime(); });
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
    j += ",\"fOk\":"    + String(s.frontTpmsOk ? 1 : 0);
    j += ",\"rOk\":"    + String(s.rearTpmsOk ? 1 : 0);
    j += ",\"fBat\":"   + String(s.frontBattPct);
    j += ",\"rBat\":"   + String(s.rearBattPct);
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

// ---------------------------------------------------------------------------
// Historia tankowań
// ---------------------------------------------------------------------------

static void appendTankJson(String& j, const TankRecord& t) {
    j += "{\"n\":" + String(t.n) + ",\"st\":" + String(t.startTime) + ",\"et\":" + String(t.endTime) +
         ",\"so\":" + String(t.startOdo, 1) + ",\"eo\":" + String(t.endOdo, 1) + ",\"km\":" + String(t.km, 2) +
         ",\"est\":" + String(t.estL, 3) + ",\"real\":" + String(t.realL, 2) +
         ",\"f0\":" + String(t.factorBefore, 3) + ",\"f1\":" + String(t.factorAfter, 3) +
         ",\"s\":" + String(t.status) + ",\"tn\":" + String(t.topN) + ",\"tl\":" + String(t.topL, 1) + ",\"ev\":[";
    for (uint8_t i = 0; i < t.nEv; i++) {
        const RefuelEvent& e = t.ev[i];
        if (i) j += ",";
        j += "{\"t\":" + String(e.time) + ",\"o\":" + String(e.odoKm, 1) + ",\"l\":" + String(e.liters, 1) +
             ",\"y\":" + String(e.type) + "}";
    }
    j += "]}";
}

void WifiPortal::handleRefuels() {
    if (!refuels) { server.send(503, "application/json", "{}"); return; }
    // Bieżący okres uzupełniamy o km i szacunek na żywo z FuelCalculator
    TankRecord open = refuels->openPeriod();
    if (fuel) { open.km = fuel->tankDistanceKm(); open.estL = fuel->tankBurnedFuelL(); }

    String j;
    j.reserve(4096);
    j += "{\"factor\":" + String(state->fuelCalFactor, 3) + ",\"open\":";
    appendTankJson(j, open);
    j += ",\"tanks\":[";
    for (uint8_t i = 0; i < refuels->tankCount(); i++) {
        if (i) j += ",";
        appendTankJson(j, refuels->tankAt(i));
    }
    j += "]}";
    server.send(200, "application/json", j);
}

// Pełna historia z karty SD; bez karty - ostatnie baki z pamięci
void WifiPortal::handleRefuelsCsv() {
    server.sendHeader("Content-Disposition", "attachment; filename=tankowania.csv");
    if (refuels && refuels->hasSd() && SD_MMC.exists("/tankowania.csv")) {
        fs::File f = SD_MMC.open("/tankowania.csv", FILE_READ);
        if (f) { server.streamFile(f, "text/csv"); f.close(); return; }
    }
    String csv = "Bak;Km;Litry_razem;Dolewki_szt;Dolewki_L;Szacunek_L100;Przeliczenie_L100;Roznica_proc;Mnoznik_przed;Mnoznik_po;Status\n";
    if (refuels) {
        for (int i = refuels->tankCount() - 1; i >= 0; i--) { // Od najstarszego
            const TankRecord& t = refuels->tankAt(i);
            float est100  = t.km > 0.1f ? t.estL / t.km * 100.0f : 0;
            float real100 = (t.km > 0.1f && t.realL > 0) ? t.realL / t.km * 100.0f : 0;
            float diff    = (t.realL > 0 && t.estL > 0) ? (t.realL / t.estL - 1.0f) * 100.0f : 0;
            const char* st = t.status == TANK_OK ? "OK" : t.status == TANK_SKIPPED ? "pominiety" : "bez_pomiaru";
            csv += String(t.n) + ";" + String(t.km, 1) + ";" + String(t.realL, 2) + ";" + String(t.topN) + ";" +
                   String(t.topL, 1) + ";" + String(est100, 2) + ";" + String(real100, 2) + ";" + String(diff, 1) + ";" +
                   String(t.factorBefore, 3) + ";" + String(t.factorAfter, 3) + ";" + st + "\n";
        }
    }
    server.send(200, "text/csv", csv);
}

// Telefon przy otwarciu strony ustawia zegar MotoDash (do dat tankowań, dopóki nie ma GPS)
void WifiPortal::handleSetTime() {
    if (server.hasArg("t")) {
        long t = server.arg("t").toInt();
        if (t > 1700000000L && RefuelLog::now() == 0) { // Ustawiamy tylko, gdy zegar nie był jeszcze ustawiony
            struct timeval tv = { (time_t)t, 0 };
            settimeofday(&tv, nullptr);
            Serial.println("[WifiPortal] Ustawiono zegar z telefonu");
        }
    }
    server.send(200, "application/json", "{\"ok\":true}");
}
