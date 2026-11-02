/* Mitori-Clickdummy: die übrigen Ansichten (Notizen, Heute üben, Prüfungsmodus, Statistik, Lehrgänge, Landkarte,
   Glossar & Suche, Einstellungen). Nutzt Zustand und Bausteine aus index.html; Lehrgänge echt aus lehrgaenge.js
   (öffentlicher Kalender), Übungsverlauf, Notizen und Prüfungsrunden sind Beispielwerte. */

/* ---------- Styles der neuen Ansichten ---------- */
document.head.insertAdjacentHTML("beforeend", `<style>
.page{padding:0 0 30px;position:relative;z-index:1}
.tab .page{max-width:880px;margin:0 auto}
.card2{margin:10px 16px;border:1px solid var(--line2);background:var(--bg);padding:12px 14px}
.card2 h3{margin:0 0 8px;font:700 11.5px var(--f-cond);letter-spacing:.1em;text-transform:uppercase;color:var(--pri);display:flex;justify-content:space-between;gap:8px}
.card2 h3 span{color:var(--fg2);letter-spacing:.04em}
.card2 p{margin:4px 0;font-size:13px;color:var(--fg2)}
.tab .grid2{display:grid;grid-template-columns:1fr 1fr;align-items:start}
.row2{display:flex;justify-content:space-between;align-items:baseline;gap:8px;font-size:13px}
.big1{font:700 44px/1 var(--f-ui);color:var(--pri);font-variant-numeric:tabular-nums}
.muted{color:var(--fg2);font-size:12.5px}
.chips{display:flex;flex-wrap:wrap;gap:5px}
.chip{background:var(--chip);padding:2px 8px;font-size:11.5px;color:var(--fg2);border:0;cursor:default}
.chip.lnk{cursor:pointer;color:var(--pri);background:color-mix(in srgb,var(--pri) 8%,var(--bg))}
.btnrow{display:flex;gap:8px;padding:0 16px}
.btnrow .btn{flex:1}
.btn.small{padding:8px;font-size:13px}
.field{display:grid;gap:3px;margin:0 16px 10px}
.field label{font:700 10.5px var(--f-cond);letter-spacing:.1em;text-transform:uppercase;color:var(--fg2)}
.field input,.field select,.field textarea{border:1px solid var(--line2);background:var(--bg2);padding:9px 10px;font:15px var(--f-ui);width:100%}
.field textarea{font:15px/1.4 var(--f-serif);min-height:70px;resize:vertical}
.tgl{display:flex;justify-content:space-between;align-items:center;gap:10px;padding:10px 16px;border-bottom:1px solid var(--bg2);font-size:14px}
.tgl small{display:block;color:var(--fg2);font-size:12px}
.sw{width:42px;height:24px;border-radius:12px;background:var(--line2);position:relative;flex:none;cursor:pointer;border:0}
.sw::after{content:"";position:absolute;top:3px;left:3px;width:18px;height:18px;border-radius:50%;background:#fff;transition:left .15s}
.sw.on{background:var(--pri)}.sw.on::after{left:21px}
.navrow{display:flex;align-items:center;gap:12px;padding:13px 16px;border-bottom:1px solid var(--line2);cursor:pointer;background:var(--bg)}
.navrow:hover{background:var(--bg2)}
.navrow .ic{font:600 20px var(--f-jp);color:var(--pri);width:26px;text-align:center}
.navrow b{display:block;font-size:15px}
.navrow span{font-size:12px;color:var(--fg2)}
.navrow em{margin-left:auto;font-style:normal;color:var(--fg2)}
.navrow.on{box-shadow:inset 3px 0 0 var(--pri);background:var(--bg2)}
.notice{margin:0 16px 10px;padding:9px 12px;background:var(--bg2);border:1px solid var(--line2);border-left:3px solid var(--pri);font-size:12.5px;display:flex;gap:10px;align-items:center;position:relative;z-index:1}
.notice span{flex:1}.notice b{color:var(--pri);cursor:pointer;white-space:nowrap}
.tab .notice{margin:0 28px 12px}
/* Notizen */
.day{margin:10px 16px;border-left:2px solid var(--line2);padding:2px 0 2px 12px}
.day .d{font:700 11px var(--f-cond);letter-spacing:.08em;text-transform:uppercase;color:var(--fg2)}
.day .d b{color:var(--fg)}
.day p{margin:4px 0 6px;font:16px/1.4 var(--f-serif)}
.day.today{border-color:var(--pri)}
/* Heatmap */
.heat{display:grid;grid-template-columns:16px repeat(var(--w),1fr);gap:2px;font:9px var(--f-ui);color:var(--fg2)}
.heat i{aspect-ratio:1;display:block;background:var(--r0)}
.heat em{font-style:normal;line-height:1;align-self:center}
.months{display:grid;grid-template-columns:16px repeat(var(--w),1fr);font:9px var(--f-ui);color:var(--fg2);margin-bottom:2px}
.lg2{display:flex;gap:4px;align-items:center;font-size:10.5px;color:var(--fg2);margin-top:6px;justify-content:flex-end}
.lg2 i{width:10px;height:10px;display:inline-block}
/* Balken je Grad */
.gbar{display:grid;grid-template-columns:64px 1fr 44px;gap:8px;align-items:center;font-size:12px;margin:5px 0}
.gbar .bar{height:12px}
.gbar b{text-align:right;font-variant-numeric:tabular-nums;font-weight:400;color:var(--fg2)}
/* Säulen Prüfungsrunden */
.cols{display:grid;grid-template-columns:22px 1fr;gap:4px;height:140px}
.yax{display:flex;flex-direction:column;justify-content:space-between;font-size:9.5px;color:var(--fg2);text-align:right;padding-bottom:16px}
.plot{position:relative;display:flex;align-items:flex-end;gap:6px;border-left:1px solid var(--line2);border-bottom:1px solid var(--line2);padding:0 4px;margin-bottom:16px}
.plot .c{flex:1;background:var(--pri);position:relative;min-width:8px}
.plot .c span{position:absolute;bottom:-15px;left:50%;transform:translateX(-50%);font-size:9px;color:var(--fg2);white-space:nowrap}
.plot .gl{position:absolute;left:0;right:0;border-top:1px dashed var(--line2)}
/* Tooltip */
.tip{position:absolute;z-index:50;background:var(--fg);color:#fff;font:12px var(--f-ui);padding:5px 8px;pointer-events:none;white-space:nowrap;transform:translate(-50%,-100%);margin-top:-8px}
.toast{position:absolute;left:50%;bottom:24px;transform:translateX(-50%);background:var(--fg);color:#fff;padding:9px 14px;font-size:13px;z-index:60;max-width:85%;text-align:center}
/* Prüfungsmodus */
.exam{flex:1;display:grid;grid-template-columns:minmax(0,1fr);place-items:center;text-align:center;padding:20px;position:relative;z-index:1}
.exam .att{font:700 12px var(--f-cond);letter-spacing:.1em;text-transform:uppercase;color:var(--fg2)}
.exam .tq{font:600 44px/1.05 var(--f-serif);margin:10px 0 6px}
.exam .jp{font-size:20px;color:var(--fg2)}
.exam .cnt{margin-top:18px;font-size:12px;color:var(--fg2)}
.exam .stich{display:inline-block;margin-top:10px;background:var(--chip);padding:3px 9px;font-size:11.5px;color:var(--fg2)}
.tab .exam .tq{font-size:72px}.tab .exam .jp{font-size:30px}
.exfoot{padding:0 16px 18px;display:grid;gap:8px;position:relative;z-index:1}
.tab .exfoot{max-width:520px;width:100%;margin:0 auto}
.rr{display:grid;grid-template-columns:1fr auto;gap:8px;align-items:center;padding:8px 16px;border-bottom:1px solid var(--bg2)}
.rr .n{font:600 15px/1.2 var(--f-serif)}
.rr .n small{display:block;font:11.5px var(--f-ui);color:var(--fg2)}
.r4{display:flex;gap:3px}
.r4 button{width:30px;height:30px;border:1px solid var(--line2);background:var(--bg);font:700 12px var(--f-ui);color:var(--fg2);cursor:pointer}
.r4 button.on{background:var(--pri);border-color:var(--pri);color:#fff}
/* Lehrgänge */
.ev{display:grid;grid-template-columns:52px 1fr auto;gap:10px;padding:10px 16px;border-bottom:1px solid var(--line2);cursor:pointer;align-items:center;background:var(--bg)}
.ev:hover{background:var(--bg2)}
.ev .dt{text-align:center;border:1px solid var(--line2);padding:3px 0;line-height:1.1}
.ev .dt b{display:block;font:700 18px var(--f-ui);color:var(--pri)}
.ev .dt span{font:700 10px var(--f-cond);letter-spacing:.08em;text-transform:uppercase;color:var(--fg2)}
.ev .t{font:600 15px/1.2 var(--f-serif)}
.ev .t small{display:block;font:12px var(--f-ui);color:var(--fg2);margin-top:2px}
.ev em{font-style:normal;font-size:11.5px;color:var(--fg2);text-align:right}
.ev.on{box-shadow:inset 3px 0 0 var(--pri);background:var(--bg2)}
.entry{padding:10px 16px;border-bottom:1px solid var(--bg2);display:grid;gap:5px}
.entry .tm{font:700 11px var(--f-cond);letter-spacing:.08em;text-transform:uppercase;color:var(--fg2)}
.entry p{margin:0;font:16px/1.4 var(--f-serif)}
.foto{height:120px;background:repeating-linear-gradient(135deg,var(--chip) 0 10px,var(--bg2) 10px 20px);display:grid;place-items:center;color:var(--fg2);font-size:12px;border:1px solid var(--line2)}
.audio{display:inline-flex;gap:8px;align-items:center;background:var(--chip);padding:5px 10px;font-size:12.5px;width:max-content}
.audio i{width:120px;height:4px;background:var(--line2);display:block;position:relative}
.audio i::after{content:"";position:absolute;left:0;top:0;bottom:0;width:30%;background:var(--pri)}
.compose{position:sticky;bottom:0;background:var(--bg);border-top:1px solid var(--line2);padding:8px 12px;display:grid;gap:6px;z-index:5}
.compose .rowc{display:flex;gap:6px;align-items:center}
.compose input{flex:1;border:1px solid var(--line2);background:var(--bg2);padding:9px 10px;font:15px var(--f-serif);min-width:0}
.compose .ib{width:38px;height:38px;border:1px solid var(--line2);background:var(--bg);cursor:pointer;font-size:16px;flex:none}
.compose .ib.on{border-color:var(--pri);color:var(--pri)}
.compose .send{border:0;background:var(--pri);color:#fff;padding:0 14px;height:38px;font:700 13px var(--f-ui);cursor:pointer}
.compose .tags{display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px}
.compose .tags input{font:13px var(--f-ui);padding:7px 8px}
.md{background:var(--bg2);border:1px solid var(--line2);padding:10px 12px;font:12px/1.5 ui-monospace,monospace;white-space:pre-wrap;max-height:330px;overflow:auto}
/* Landkarte */
.mxw{overflow:auto;margin:8px 0 0;padding:0 16px 8px}
.mx{border-collapse:separate;border-spacing:2px;font-size:11px}
.mx th{font-weight:400;color:var(--fg2);text-align:left;white-space:nowrap}
.mx thead th{height:110px;vertical-align:bottom;padding:0}
.mx thead th div{writing-mode:vertical-rl;transform:rotate(180deg);font-size:10.5px;max-height:110px;overflow:hidden}
.mx tbody th{position:sticky;left:0;background:var(--bg);padding-right:6px;font:600 13px var(--f-serif);color:var(--fg)}
.mx td{width:26px;height:26px;text-align:center;font:700 10px var(--f-ui);cursor:pointer;color:#fff}
.mx td.r0{background:var(--r0);color:var(--fg2)}.mx td.r1{background:var(--r1);color:var(--fg)}.mx td.r2{background:var(--r2)}.mx td.r3{background:var(--r3)}.mx td.r4{background:var(--r4)}
.mx td.no{background:none;cursor:default}
.tab .mx td{width:34px;height:34px}
.formgrid{display:grid;grid-template-columns:repeat(8,1fr);gap:4px;padding:6px 16px}
.formgrid button{aspect-ratio:1;border:0;font:700 12px var(--f-ui);cursor:pointer}
.tab .formgrid{grid-template-columns:repeat(11,1fr)}
/* Suche */
.search{margin:0 16px 8px;display:flex;border:1px solid var(--line2);background:var(--bg2);align-items:center}
.search input{flex:1;border:0;background:none;padding:11px 12px;font:16px var(--f-ui);min-width:0;outline:none}
.search span{padding:0 12px;color:var(--fg2)}
.res{padding:9px 16px;border-bottom:1px solid var(--bg2);cursor:pointer;display:grid;grid-template-columns:1fr auto;gap:8px;align-items:center}
.res:hover{background:var(--bg2)}
.res .n{font:600 15px/1.25 var(--f-serif)}
.res .n small{display:block;font:12px/1.35 var(--f-ui);color:var(--fg2)}
.res mark{background:color-mix(in srgb,var(--pri) 18%,var(--bg));color:inherit}
.say{width:32px;height:32px;border-radius:50%;border:1px solid var(--line2);background:var(--bg);cursor:pointer;color:var(--pri)}
.az{display:flex;flex-wrap:wrap;gap:2px;padding:0 16px 6px}
.az button{border:0;background:none;font:700 12px var(--f-ui);color:var(--pri);cursor:pointer;padding:3px 5px}
/* Einstellungen */
.store{display:flex;height:10px;gap:1px;margin:6px 0}
.store i{display:block;height:100%}
.bd{padding:11px 16px;border-bottom:1px solid var(--line2);display:grid;grid-template-columns:1fr auto;gap:4px 10px;align-items:center;background:var(--bg)}
.bd b{font-size:14.5px}
.bd .muted{grid-column:1}
.bd .act{grid-row:1/3;grid-column:2;display:grid;gap:4px;justify-items:end}
.bd .st{font:700 10.5px var(--f-cond);letter-spacing:.08em;text-transform:uppercase}
.bd .st.ok{color:#2f7d3b}.bd .st.up{color:var(--pri)}.bd .st.no{color:var(--fg2)}
.prog2{height:4px;background:var(--line2);grid-column:1/-1;position:relative}
.prog2 i{position:absolute;left:0;top:0;bottom:0;background:var(--pri)}
.codein{margin:6px 16px;font:600 22px ui-monospace,monospace;letter-spacing:.06em;text-transform:uppercase;text-align:center;border:1px solid var(--line2);background:var(--bg2);padding:14px 8px;width:calc(100% - 32px)}
.msg{margin:6px 16px;padding:9px 12px;font-size:13px}
.msg.err{background:color-mix(in srgb,var(--pri) 10%,var(--bg));color:var(--pri)}
.msg.ok{background:#e7f2e8;color:#2f7d3b}
.pk{width:100%;max-width:560px;text-align:center}
.pk .grp{font:700 12px var(--f-cond);letter-spacing:.12em;text-transform:uppercase;color:var(--fg2);margin-bottom:10px}
.pk .jp{font:400 15px var(--f-jp);color:var(--fg2);margin-top:2px}
.pk .pz{font:400 .6em var(--f-ui);color:var(--fg2);white-space:nowrap}
.pk .lb{font:700 10.5px var(--f-cond);letter-spacing:.14em;text-transform:uppercase;color:var(--pri);margin-top:6px}
.pkD table{width:100%;border-collapse:collapse;text-align:left}
.pkD th{font:700 10.5px var(--f-cond);letter-spacing:.14em;text-transform:uppercase;color:var(--pri);vertical-align:top;padding:14px 12px 14px 0;width:1%;white-space:nowrap;border-top:1px solid var(--line2)}
.pkD td{padding:10px 0;border-top:1px solid var(--line2)}.pkD td.a{font:600 28px/1.1 var(--f-serif)}.pkD td.t{font:600 40px/1.05 var(--f-serif)}
.tab .pk{max-width:900px}.tab .pkD td.a{font-size:46px}.tab .pkD td.t{font-size:68px}
.tab .pk .jp{font-size:22px}.tab .pk .grp{font-size:15px}.tab .pk .lb,.tab .pkD th{font-size:13px}
.split2{flex:1;display:flex;min-height:0}
.split2 > .l{flex:0 0 400px;border-right:1px solid var(--line2)}
.split2 > .r{flex:1;background:var(--bg2)}
</style>`);

/* ---------- Hilfen ---------- */
const TAG = 86400000;
const heute0 = () => { const d = new Date(); d.setHours(0, 0, 0, 0); return d; };
const isoD = d => `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, "0")}-${String(d.getDate()).padStart(2, "0")}`;
const parseD = s => { const [y, m, d] = s.split("-").map(Number); return new Date(y, m - 1, d); };
const kurzD = d => d.toLocaleDateString("de-DE", { weekday: "short", day: "numeric", month: "numeric" });
const langD = d => d.toLocaleDateString("de-DE", { weekday: "long", day: "numeric", month: "long", year: "numeric" });
const LE = window.LEHRGAENGE || [];
S.log = S.log || {};            // iso-Datum → geübte Ids (heute)
S.prHist = S.prHist || null;    // Prüfungsrunden
S.kurse = S.kurse || null;      // Lehrgangs-Notizbücher
S.bundleVer = S.bundleVer || {};
S.upd = S.upd || { app: false, video: true };
S.theme = S.theme || "shinki";
S.sync = S.sync || { webdav: true, auto: true, backup: true };

function toast(msg){ const t = document.createElement("div"); t.className = "toast"; t.textContent = msg; $("#scr").appendChild(t); setTimeout(() => t.remove(), 2200); }

/* Tooltips für Diagramme: data-tip an jedem Wert */
document.addEventListener("mouseover", ev => {
  const el = ev.target.closest("[data-tip]"), scr = $("#scr"), old = scr.querySelector(".tip");
  if (old) old.remove();
  if (!el || !scr.contains(el)) return;
  const r = el.getBoundingClientRect(), s = scr.getBoundingClientRect(), tip = document.createElement("div");
  tip.className = "tip"; tip.textContent = el.dataset.tip;
  tip.style.left = (r.left + r.width / 2 - s.left) + "px"; tip.style.top = (r.top - s.top) + "px";
  scr.appendChild(tip);
});

/* ---------- Beispieldaten: Übungsverlauf ---------- */
function geuebtAm(iso){
  const d = parseD(iso), wd = d.getDay(), h = hash("tag" + iso);
  if (iso === isoD(heute0())) return (S.log[iso] || []).length;
  if (d > heute0()) return 0;
  if (wd === 2 || wd === 4) return h < .85 ? 3 + Math.floor(hash("n" + iso) * 12) : 0;
  return h < .08 ? 2 + Math.floor(hash("n" + iso) * 5) : 0;
}
function tage30(){ let n = 0; for (let k = 0; k < 30; k++) if (geuebtAm(isoD(new Date(heute0() - k * TAG)))) n++; return n; }
function letzteUebung(id){ return Math.floor(hash(id + "z") * 40); } // Tage seit zuletzt geübt (Beispiel)
function logge(id){ const k = isoD(heute0()); (S.log[k] = S.log[k] || []).includes(id) || S.log[k].push(id); }

/* Wiederholen nach Plan: Unsicheres öfter, lange nicht Geübtes, dazu Neues aus dem aktuellen Grad, für die Sparte
   der heutigen Einheit. Abstände je Stufe einstellbar. Lehrgangsnotizen zählen nicht mit (entschieden). */
const DISC = [["aikido", "Aikido"], ["daitoryu", "Daitoryu"], ["shinkitoho", "Shinki Toho"]];
S.heuteSp = typeof S.heuteSp === "string" ? S.heuteSp : "aikido";
S.abst = S.abst || [0, 2, 4, 14, 35]; // Tage bis wieder fällig, je Stufe 1–4
function faellig(){
  const did = S.heuteSp, d = disc(did), g = S.grade[did], order = d.grade.map(x => x.id), bis = order.indexOf(g), teil = [];
  for (const gid of order.slice(0, bis + 1)) for (const e of uebbar(eintraege(did, gid))) {
    const r = rating(e.id), tage = letzteUebung(e.id);
    if ((S.log[isoD(heute0())] || []).includes(e.id)) continue;
    if (r === 0) { if (gid === g) teil.push({ e, did, prio: 1, grund: "neu im " + (d.grade[bis] || {}).name }); continue; }
    if (tage >= S.abst[r]) teil.push({ e, did, prio: (5 - r) * 10 + tage / 10, grund: r <= 2 ? `${RATING[r]} · vor ${tage} Tagen` : `seit ${tage} Tagen nicht geübt` });
  }
  return [...teil.filter(x => x.prio !== 1).sort((a, b) => b.prio - a.prio).slice(0, 8), ...teil.filter(x => x.prio === 1).slice(0, 2)];
}

/* ---------- Menü: Hinweise auf neue App-Version und neue Videoversion ---------- */
function hinweise(){
  let h = "";
  if (S.upd.app) h += `<div class="notice"><span>Mitori 1.1.0 ist erschienen.</span><b onclick="S.upd.app=false;save();render()">Später</b><b onclick="toast('Öffnet die Release-Seite auf GitHub')">Ansehen ›</b></div>`;
  if (S.upd.video && installiert("aikido-kyu") && (S.bundleVer["aikido-kyu"] || 1) < 2) h += `<div class="notice"><span>Neue Videoversion: <b style="color:var(--fg);cursor:default">Aikido · alle Kyu</b> (v2)</span><b onclick="go('videos')">Ansehen ›</b></div>`;
  return h;
}
function naechsterLehrgang(){
  const ex = naechstePruefung(); if (ex) return `Prüfung ${ex.text} in ${ex.tage} Tagen`;
  const n = LE.find(x => parseD(x.von) >= heute0());
  if (!n) return "keine Termine";
  const tage = Math.round((parseD(n.von) - heute0()) / TAG);
  return `${n.ort || n.titel} · in ${tage} Tagen`;
}

/* ---------- Notizen je Technik und Übungstag ---------- */
const BEISPIEL_NOTIZ = ["Hüfte früher drehen, Arm bleibt vor dem Kopf.", "Eintritt zu weit nach außen, Abstand kleiner wählen.", "Uke nicht nach unten ziehen, Kreis größer.", "Blick bleibt beim Partner, nicht auf die Hände.", "Gewicht vorne lassen, nicht zurücklehnen."];
const BEISPIEL_ST = ["Kissaki bleibt in der Mitte, nicht zu früh ausholen.", "Schritt und Schlag gleichzeitig, nicht nacheinander.", "Zanshin länger halten."];
/* Notizen: eigene (S.notes, Text mit optionalem Foto/Sprachnotiz), Beispielverlauf und verknüpfte Lehrgangseinträge.
   Lang drücken öffnet Bearbeiten/Löschen; Änderungen an Beispielnotizen liegen in S.nEdit / S.nWeg. */
S.nEdit = S.nEdit || {}; S.nWeg = S.nWeg || {};
const nObj = n => typeof n === "string" ? { text: n } : n;
function notizenVon(id){
  const r = rating(id), e = findEintrag(id), pool = e && e.disc === "shinkitoho" ? BEISPIEL_ST : BEISPIEL_NOTIZ;
  const out = (S.notes[id] || []).map((n, i) => Object.assign({ d: heute0(), tag: "heute", key: `own|${id}|${i}` }, nObj(n)));
  if (r) {
    const t0 = letzteUebung(id), p0 = Math.floor(hash(id) * pool.length);
    out.push({ d: new Date(heute0() - t0 * TAG), text: pool[p0], tag: "Bewertung: " + RATING[r], key: `smp|${id}|0` });
    out.push({ d: new Date(heute0() - (t0 + 7) * TAG), text: pool[(p0 + 1) % pool.length], tag: "Bewertung: " + RATING[Math.max(1, r - 1)], key: `smp|${id}|1`, foto: hash(id + "f") < .3 });
  }
  for (const k of Object.values(kurse())) k.eintraege.forEach((en, i) => { if (en.link === id) out.push({ d: parseD(k.von), text: en.text, foto: en.foto, audio: en.audio, tag: "aus Lehrgang " + (k.ort || k.titel), kurs: k.key, key: `kurs|${k.key}|${i}` }); });
  return out.filter(n => !S.nWeg[n.key]).map(n => S.nEdit[n.key] != null ? Object.assign(n, { text: S.nEdit[n.key] }) : n).sort((a, b) => b.d - a.d);
}
const anhang = n => `${n.foto ? `<div class="foto">Foto / Skizze</div>` : ""}${n.audio ? `<span class="audio">▶ <i></i> ${esc(n.audio)}</span>` : ""}`;
V.notizen = id => {
  const e = findEintrag(id), ns = notizenVon(id), tage = {};
  ns.forEach(n => (tage[isoD(n.d)] = tage[isoD(n.d)] || []).push(n));
  return `${sbar()}<div class="body" data-scroll="notizen" style="display:flex;flex-direction:column"><div class="ink"></div>${backHd("Zurück", "Notizen", "記", `<button class="iconbtn" title="Alle Notizen" onclick="go('allenotizen')">☰</button>`)}
   <div class="page" style="flex:1">
    <div class="card2"><div class="row2"><div><div class="k" style="font:700 11px var(--f-cond);letter-spacing:.1em;text-transform:uppercase;color:var(--pri)">${esc(e.meta || e.gruppe.split("|")[0])}</div><b style="font:600 20px var(--f-serif)">${e.nr ? e.nr + " · " : ""}${esc(e.title)}</b> <span class="jp muted">${esc(e.jp || "")}</span></div>${dots(rating(id))}</div></div>
    ${Object.entries(tage).map(([iso, l]) => `<div class="day ${iso === isoD(heute0()) ? "today" : ""}"><div class="d"><b>${iso === isoD(heute0()) ? "Heute" : kurzD(parseD(iso))}</b> · ${l.some(n => n.kurs) ? "Lehrgang" : "Training"}</div>${l.map(n => `<div class="nz" data-key="${esc(n.key)}"><p>${esc(n.text)}</p>${anhang(n)}<span class="chip ${n.kurs ? "lnk" : ""}" ${n.kurs ? `onclick="go('lehrgang','${n.kurs}')"` : ""}>${esc(n.tag)}</span></div>`).join("")}</div>`).join("") || `<p class="muted" style="padding:0 16px">Noch keine Notizen zu dieser Technik.</p>`}
    <p class="muted" style="padding:4px 16px">Lang drücken: bearbeiten oder löschen.</p>
   </div>
   <div class="compose"><div class="rowc"><button class="ib" title="Foto" onclick="notizNeu('${id}','foto')">📷</button><button class="ib" title="Sprachnotiz" onclick="notizNeu('${id}','audio')">🎤</button>
    <input id="qn2" placeholder="Notiz für heute, ${today()} …" onkeydown="if(event.key==='Enter')notizNeu('${id}')"><button class="send" onclick="notizNeu('${id}')">+</button></div></div></div>`;
};
function notizNeu(id, art){
  const x = ($("#qn2")?.value || "").trim(); if (!x && !art) return;
  (S.notes[id] = S.notes[id] || []).push({ text: x || (art === "foto" ? "Foto" : "Sprachnotiz"), foto: art === "foto", audio: art === "audio" ? "0:14" : null });
  logge(id); save(); render();
}
/* Lang drücken (oder Rechtsklick) auf einer Notiz */
let DRUECK = null;
document.addEventListener("pointerdown", ev => { const el = ev.target.closest("#scr .nz"); if (!el || ev.target.closest(".chip")) return; DRUECK = setTimeout(() => notizMenue(el.dataset.key), 500); });
["pointerup", "pointerleave", "pointercancel", "scroll"].forEach(n => document.addEventListener(n, () => clearTimeout(DRUECK), true));
document.addEventListener("contextmenu", ev => { const el = ev.target.closest("#scr .nz"); if (!el) return; ev.preventDefault(); clearTimeout(DRUECK); notizMenue(el.dataset.key); });
function notizText(key){
  const [art, a, i] = key.split("|");
  if (S.nEdit[key] != null) return S.nEdit[key];
  if (art === "own") return nObj(S.notes[a][+i]).text;
  if (art === "kurs") return kurse()[a].eintraege[+i].text;
  return (notizenVon(a).find(n => n.key === key) || {}).text || "";
}
function notizMenue(key, bearbeiten){
  closeSheet();
  const el = document.createElement("div");
  el.className = "dim"; el.onclick = ev => { if (ev.target === el) el.remove(); };
  el.innerHTML = bearbeiten
    ? `<div class="sheet"><div class="grab"></div><div class="k">Notiz bearbeiten</div><div class="field" style="margin:0"><textarea id="ned">${esc(notizText(key))}</textarea></div>
       <div class="btns"><button class="btn" onclick="notizSpeichern('${esc(key)}')">Speichern</button><button class="btn ghost" onclick="closeSheet()">Abbrechen</button></div></div>`
    : `<div class="sheet"><div class="grab"></div><div class="lastnote">${esc(notizText(key))}</div>
       <div class="btns"><button class="btn ghost" onclick="notizMenue('${esc(key)}',true)">Bearbeiten</button><button class="btn ghost" style="color:var(--pri)" onclick="notizLoeschen('${esc(key)}')">Löschen</button></div></div>`;
  $("#scr").appendChild(el);
}
function notizSpeichern(key){
  const [art, a, i] = key.split("|"), x = $("#ned").value.trim();
  if (art === "own") S.notes[a][+i] = Object.assign(nObj(S.notes[a][+i]), { text: x });
  else if (art === "kurs") kurse()[a].eintraege[+i].text = x;
  else S.nEdit[key] = x;
  save(); closeSheet(); render();
}
function notizLoeschen(key){
  const [art, a, i] = key.split("|");
  if (art === "own") S.notes[a].splice(+i, 1);
  else if (art === "kurs") kurse()[a].eintraege.splice(+i, 1);
  else S.nWeg[key] = true;
  save(); closeSheet(); render(); toast("Notiz gelöscht");
}
/* Alle Notizen über alle Techniken und Lehrgänge, nach Tag */
S.anFilter = S.anFilter || "alle";
V.allenotizen = () => {
  const alle = [];
  for (const d of D.disziplinen) for (const g of d.grade) for (const e of uebbar(eintraege(d.id || d.kurz, g.id)))
    if (rating(e.id) || (S.notes[e.id] || []).length) for (const n of notizenVon(e.id)) if (!n.kurs) alle.push(Object.assign({ e, sparte: d.id || d.kurz }, n));
  for (const k of Object.values(kurse())) k.eintraege.forEach((en, i) => { if (!S.nWeg[`kurs|${k.key}|${i}`]) alle.push({ d: parseD(k.von), text: en.text, foto: en.foto, audio: en.audio, kurs: k, key: `kurs|${k.key}|${i}`, tm: en.tm }); });
  const f = alle.filter(n => S.anFilter === "alle" || (S.anFilter === "lehrgang" ? n.kurs : n.sparte === S.anFilter)).sort((a, b) => b.d - a.d);
  const tage = {}; f.forEach(n => (tage[isoD(n.d)] = tage[isoD(n.d)] || []).push(n));
  return `${sbar()}<div class="body" data-scroll="alle"><div class="ink"></div>${backHd("Zurück", "Alle Notizen", "記録")}
   <div class="page">
    <div class="chips" style="padding:0 16px 8px">${[["alle", "Alle"], ...DISC, ["lehrgang", "Lehrgänge"]].map(([id, t]) => `<button class="pill ${S.anFilter === id ? "on" : ""}" onclick="S.anFilter='${id}';neuListe();save();render()">${t}</button>`).join("")}</div>
    <p class="muted" style="padding:0 16px">${f.length} Notizen. Tippen öffnet die Technik bzw. das Notizbuch, lang drücken bearbeitet.</p>
    ${Object.entries(tage).slice(0, 40).map(([iso, l]) => `<div class="day ${iso === isoD(heute0()) ? "today" : ""}"><div class="d"><b>${iso === isoD(heute0()) ? "Heute" : kurzD(parseD(iso))}</b></div>${l.map(n => `<div class="nz" data-key="${esc(n.key)}" style="cursor:pointer" onclick="${n.kurs ? `kursOeffnen('${n.kurs.key}')` : `go('notizen','${n.e.id}')`}"><span class="chip ${n.kurs ? "lnk" : ""}" style="margin-top:4px">${n.kurs ? esc(n.kurs.titel) + " · " + esc(n.tm) : esc(n.e.title) + (n.e.angriff ? " · " + esc(n.e.angriff.name) : "")}</span><p>${esc(n.text)}</p>${anhang(n)}</div>`).join("")}</div>`).join("")}
   </div></div>`;
};

/* ---------- Heute üben ---------- */
function heatmap(wochen){
  const t = heute0(), start = new Date(t - ((wochen - 1) * 7 + ((t.getDay() + 6) % 7)) * TAG); // Montag
  const zellen = [], monate = [];
  let lastM = -1;
  for (let w = 0; w < wochen; w++) {
    const d0 = new Date(+start + w * 7 * TAG);
    monate.push(d0.getMonth() !== lastM ? d0.toLocaleDateString("de-DE", { month: "short" }) : "");
    lastM = d0.getMonth();
  }
  for (let wd = 0; wd < 7; wd++) {
    zellen.push(`<em>${wd % 2 === 0 ? "MDMDFSS"[wd] : ""}</em>`);
    for (let w = 0; w < wochen; w++) {
      const d = new Date(+start + (w * 7 + wd) * TAG), n = d > t ? -1 : geuebtAm(isoD(d));
      const stufe = n <= 0 ? 0 : n < 5 ? 1 : n < 9 ? 2 : n < 13 ? 3 : 4;
      zellen.push(n < 0 ? `<i style="background:none"></i>` : `<i style="background:var(--r${stufe})" data-tip="${kurzD(d)}: ${n ? n + " Techniken" : "kein Training"}"></i>`);
    }
  }
  return `<div class="months" style="--w:${wochen}"><span></span>${monate.map(m => `<span>${m}</span>`).join("")}</div>
   <div class="heat" style="--w:${wochen}">${zellen.join("")}</div>
   <div class="lg2">weniger <i style="background:var(--r0)"></i><i style="background:var(--r1)"></i><i style="background:var(--r2)"></i><i style="background:var(--r3)"></i><i style="background:var(--r4)"></i> mehr Techniken je Tag</div>`;
}
V.heute = () => {
  const f = faellig(), ex = naechstePruefung();
  return `${sbar()}<div class="body" data-scroll="heute"><div class="ink"></div>${backHd("Menü", "Heute üben", "稽古")}
   <div class="page">
    <div class="card2"><h3>Heute übe ich</h3><div class="seg" style="margin:0">${DISC.map(([id, t]) => `<button class="${S.heuteSp === id ? "on" : ""}" onclick="S.heuteSp='${id}';save();render()">${t}</button>`).join("")}</div><p>bis ${esc(disc(S.heuteSp).grade.find(x => x.id === S.grade[S.heuteSp]).name)} (wie im Programm gewählt)</p></div>
    ${ex ? `<div class="notice"><span>Prüfung <b style="color:var(--fg);cursor:default">${esc(ex.text)}</b> in ${ex.tage} Tagen · ${esc(ex.ort)}</span><b onclick="S.disc='${ex.disc}';S.grade['${ex.disc}']='${ex.grade}';save();go('pruefung')">Probeprüfung ›</b></div>` : ""}
    <div class="grid2">
    <div class="card2"><div class="row2"><span class="big1">${f.length}</span><span class="muted" style="text-align:right">Techniken fällig</span></div>
     <p>${f.filter(x => rating(x.e.id) <= 2 && rating(x.e.id) > 0).length} unsicher · ${f.filter(x => rating(x.e.id) >= 3).length} lange nicht geübt · ${f.filter(x => !rating(x.e.id)).length} neu</p>
     ${f.length ? `<button class="btn" style="margin-top:8px" onclick="S.sess={ids:${esc(JSON.stringify(f.map(x => x.e.id)))},i:0,done:[]};save();go('session')">Training starten</button>` : `<p>Für heute ist alles geübt.</p>`}</div>
    <div class="card2"><h3>Übungstage <span>12 Wochen</span></h3>${heatmap(12)}</div>
   </div>
   <div class="gh">Fällig<span class="jp">tippen öffnet das Blatt</span></div>${f.map(x => `<div class="li" onclick="sheet('${x.e.id}')"><div class="n">${x.e.nr ? `<span class="nr">${x.e.nr}</span>` : ""}${esc(x.e.title)}<small>${esc(x.e.angriff ? x.e.angriff.name + " · " : "")}${esc(x.grund)}</small></div>${dots(rating(x.e.id))}<span></span></div>`).join("")}
   <div class="card2" style="margin-top:16px"><h3>Abstände <span>Tage bis wieder fällig</span></h3>
    <div style="display:grid;grid-template-columns:repeat(4,1fr);gap:6px">${[1, 2, 3, 4].map(r => `<label class="field" style="margin:0"><label>${r} ${RATING[r]}</label><input type="number" min="1" max="120" value="${S.abst[r]}" onchange="S.abst[${r}]=Math.max(1,+this.value||1);save();render()"></label>`).join("")}</div>
    <p>Neue Techniken des aktuellen Grades kommen zusätzlich, höchstens zwei am Tag.</p></div>
   </div></div>`;
};
/* Training: eine Playliste nur aus den heutigen Techniken. Einschätzen schaltet zur nächsten offenen Technik;
   vorige/nächste und ⤢ bleiben in der Playliste. Ohne Video: Karte mit Name und letzter Notiz. */
const inSess = () => !!(S.sess && S.sess.ids && S.stack.some(r => r[0] === "session"));
function sessRate(id, k){
  S.ratings[id] = k; logge(id);
  if (!S.sess.done.includes(id)) S.sess.done.push(id);
  const s = S.sess, offen = n => !s.done.includes(s.ids[n]);
  let n = s.ids.findIndex((x, j) => j > s.i && offen(j)); if (n < 0) n = s.ids.findIndex((x, j) => offen(j));
  s.i = n < 0 ? s.ids.length : n;
  if (S.stack.at(-1)[0] !== "session") S.stack.pop(); // aus Player groß zurück in die Playliste
  save(); render();
}
function sessListe(){
  const s = S.sess;
  return `<div class="gh">Playliste heute<span class="jp">${s.done.length} von ${s.ids.length} geübt</span></div>${s.ids.map((id, n) => { const e = findEintrag(id), fertig = s.done.includes(id);
    return `<div class="li ${n === s.i ? "sel" : ""}" onclick="S.sess.i=${n};save();render()"><div class="n"><span class="nr">${fertig ? "✓" : n + 1}</span>${esc(e.title)}<small>${esc(e.angriff ? e.angriff.name + (e.angriff.zusatz ? " · " + e.angriff.zusatz : "") : e.gruppe.split("|")[0])}</small></div>${dots(rating(id))}<span></span></div>`; }).join("")}`;
}
function sessKarte(id){
  const e = findEintrag(id), r = rating(id);
  return `<div class="card2" style="padding:20px 16px"><div class="k" style="font:700 11px var(--f-cond);letter-spacing:.1em;text-transform:uppercase;color:var(--pri)">${esc(e.meta || e.gruppe.split("|")[0])} · ${S.sess.i + 1} / ${S.sess.ids.length}</div>
     <div style="font:600 30px/1.1 var(--f-serif);margin:6px 0 2px">${e.nr ? e.nr + " · " : ""}${esc(e.title)}</div><div class="jp muted" style="font-size:16px">${esc(e.jp || "")}</div>
     ${r ? `<div class="lastnote" style="margin-top:12px"><span>Letzte Notiz</span>${esc(notizenVon(id)[0]?.text || "")}</div>` : ""}<p>Für diesen Grad ist kein Video geladen.</p></div>
    <div class="prate"><div class="q">Wie lief es heute?</div><div class="rate">${[1,2,3,4].map(k => `<button class="${k === r ? "on" : ""}" onclick="sessRate('${id}',${k})"><b>${k}</b>${RATING[k]}</button>`).join("")}</div></div>`;
}
V.session = () => {
  if (!S.sess) S.sess = { ids: faellig().map(x => x.e.id), i: 0, done: [] };
  const s = S.sess;
  if (!s || s.i >= s.ids.length) return `${sbar()}<div class="body"><div class="ink"></div>${backHd("Heute üben", "Geschafft", "")}
    <div class="page"><div class="card2"><span class="big1">${s ? s.done.length : 0}</span><p>Techniken geübt und eingeschätzt. Sie zählen als Übungstag in der Statistik.</p></div>
    <div class="btnrow"><button class="btn" onclick="back()">Zurück zu Heute</button></div></div></div>`;
  const id = s.ids[s.i], e = findEintrag(id), vid = hasVideo(e.disc, e.grade);
  if (T()) return `${sbar()}<div class="split2"><div class="l body" data-scroll="pl"><div class="ink"></div>${backHd("Heute üben", "Training", "稽古")}${sessListe()}</div>
    ${vid ? playerPane(id) : `<div class="r body"><div class="page" style="padding-top:14px">${sessKarte(id)}</div></div>`}</div>`;
  if (vid) return V.player(id);
  return `${sbar()}<div class="body"><div class="ink"></div>${backHd("Heute üben", "Training", "")}<div class="page">${sessKarte(id)}${sessListe()}</div></div>`;
};

/* ---------- Prüfungsmodus ---------- */
S.prSet = Object.assign({ n: 0, zufall: false, frueher: true }, S.prSet || {}); // n = 0: alle
const swb = (on, js) => `<button class="sw ${on ? "on" : ""}" onclick="${js}"></button>`;
/* Prüfungsstoff eines Grades. Shinki Toho: Shoden = Kamae (als ein Punkt) und Ô-dachi 1–11, danach nur die Formen. */
const KAMAE_PR = { id: "shinkitoho/kamae", disc: "shinkitoho", grade: "shoden", title: "Kamae", jp: "構", gruppe: "Kamae|構", meta: "Shoden" };
const _findEintrag = findEintrag;
findEintrag = id => id === KAMAE_PR.id ? KAMAE_PR : _findEintrag(id);
function prStoff(did, g){
  const es = uebbar(eintraege(did, g)).map(e => Object.assign({ disc: did }, e));
  if (did !== "shinkitoho") return es;
  const formen = es.filter(e => e.id.startsWith("shinkitoho/odachi/"));
  return g === "shoden" ? [KAMAE_PR, ...formen] : formen;
}
V.pruefung = () => {
  const d = disc(S.disc), g = S.grade[S.disc], gname = d.grade.find(x => x.id === g).name;
  const anz = prStoff(S.disc, g).length, ex = naechstePruefung(S.disc);
  const letzte = prHist().filter(h => h.disc === S.disc).slice(-1)[0];
  const hatFrueher = d.grade.findIndex(x => x.id === g) > 0;
  return `${sbar()}<div class="body"><div class="ink"></div>${backHd("Menü", "Prüfungsmodus", "試験")}
   <div class="page">
    ${ex ? `<div class="notice"><span>Geplant: <b style="color:var(--fg);cursor:default">${esc(ex.text)}</b> in ${ex.tage} Tagen · ${esc(ex.ort)}</span>${ex.grade !== g ? `<b onclick="S.grade[S.disc]='${ex.grade}';save();render()">diesen Grad ›</b>` : ""}</div>` : ""}
    <div class="seg">${DISC.map(([id,t]) => `<button class="${id === S.disc ? "on" : ""}" onclick="S.disc='${id}';save();render()">${t}</button>`).join("")}</div>
    <div class="grades">${d.grade.filter(x => prStoff(S.disc, x.id).length).map(x => `<button class="gchip ${x.id === g ? "on" : ""}" onclick="S.grade[S.disc]='${x.id}';save();render()"><b>${esc(x.name)}</b><span>${prStoff(S.disc, x.id).length} Punkte</span></button>`).join("")}</div>
    <div class="card2" style="padding:0"><h3 style="padding:12px 16px 4px">Probe ${esc(gname)}</h3>
     <div class="tgl"><span>Anzahl aus dem ${esc(gname)}</span><span class="r4">${[0, 8, 12, 15].filter(v => v < anz).map(v => `<button class="${S.prSet.n === v ? "on" : ""}" style="width:auto;padding:0 8px" onclick="S.prSet.n=${v};save();render()">${v || "alle " + anz}</button>`).join("")}</span></div>
     <div class="tgl"><span>Zufällige Reihenfolge<small>aus: wie in der Prüfungsordnung</small></span>${swb(S.prSet.zufall, "S.prSet.zufall=!S.prSet.zufall;save();render()")}</div>
     <div class="tgl" style="border:0;${hatFrueher ? "" : "opacity:.45"}"><span>Techniken aus früheren Graden<small>${hatFrueher ? "2–5 Stichproben, immer am Anfang" : "erster Grad, keine früheren"}</small></span>${swb(S.prSet.frueher && hatFrueher, hatFrueher ? "S.prSet.frueher=!S.prSet.frueher;save();render()" : "")}</div>
    </div>
    <p class="muted" style="padding:0 16px">Eingeblendet wird nur, was geprüft wird (bei Shinki Toho nur die Technik), kein Video, keine Sprachausgabe. Am Ende schätzt du jede Technik ein; das setzt ihren Stand und geht in die Statistik.${letzte ? ` Letzte Runde: ${kurzD(new Date(letzte.d))}, Schnitt ${letzte.schnitt.toFixed(1).replace(".", ",")}.` : ""}</p>
    <div class="btnrow"><button class="btn" onclick="prStart()">Runde starten</button></div>
   </div></div>`;
};
function prStart(ohneGo){
  const d = disc(S.disc), g = S.grade[S.disc], order = d.grade.map(x => x.id), i = order.indexOf(g);
  const mix = a => a.map(x => [Math.random(), x]).sort((p, q) => p[0] - q[0]).map(x => x[1]);
  const alle = prStoff(S.disc, g);
  let haupt = S.prSet.n ? mix(alle).slice(0, S.prSet.n) : alle.slice();
  haupt = S.prSet.zufall ? mix(haupt) : haupt.sort((a, b) => alle.indexOf(a) - alle.indexOf(b)); // sonst Reihenfolge der Prüfungsordnung
  const frueher = order.slice(0, i).flatMap(x => prStoff(S.disc, x));
  const stich = S.prSet.frueher && frueher.length ? mix(frueher).slice(0, 2 + Math.floor(Math.random() * 4)).map(e => ({ id: e.id, stich: true })) : [];
  S.pr = { liste: [...stich, ...haupt.map(e => ({ id: e.id }))], i: 0, erg: {} }; // Stichproben immer am Anfang
  save(); if (!ohneGo) go("runde");
}
/* Anzeige einer Prüfungstechnik als Tafel: Gruppe (Stand/Sitz), Angriff, Technik; Shinki Toho nur die Technik */
function pruefKarte(e){
  const shinki = e.disc === "shinkitoho" || e.id.startsWith("shinkitoho/");
  const zus = x => x ? ` <span class="pz">${esc(x)}</span>` : "";
  const tech = `${e.nr ? e.nr + " · " : ""}${esc(e.title)}${zus(e.extra)}<div class="jp">${esc(e.jp || "")}</div>`;
  if (shinki) return `<div class="pk pkD"><table><tr><th>Technik</th><td class="t">${tech}</td></tr></table></div>`;
  return `<div class="pk pkD"><div class="grp">${esc(e.gruppe.split("|")[0])}</div><table>${e.angriff ? `<tr><th>Angriff</th><td class="a">${esc(e.angriff.name)}${zus(e.angriff.zusatz)}${e.angriff.jp ? `<div class="jp">${esc(e.angriff.jp)}</div>` : ""}</td></tr>` : ""}<tr><th>Technik</th><td class="t">${tech}</td></tr></table></div>`;
}
V.runde = () => {
  if (!S.pr || !S.pr.liste[S.pr.i]) prStart(true);
  const p = S.pr, x = p.liste[p.i], e = findEintrag(x.id);
  return `${sbar()}<div class="body" style="display:flex;flex-direction:column"><div class="ink"></div>${backHd("Abbrechen", "Probeprüfung", "", `<span class="muted">${p.i + 1} / ${p.liste.length}</span>`)}
   <div class="exam">${pruefKarte(e)}${x.stich ? `<span class="stich">Stichprobe ${esc(findGrad(e))}</span>` : ""}</div>
   <div class="exfoot"><button class="btn" onclick="S.pr.i++;save();S.pr.i>=S.pr.liste.length?(S.stack[S.stack.length-1]=['pruefende'],save(),render()):render()">Weiter</button><span class="muted" style="text-align:center">Bewertung aller Techniken am Ende der Runde</span></div></div>`;
};
V.pruefende = () => {
  const p = S.pr;
  return `${sbar()}<div class="body" data-scroll="pe"><div class="ink"></div>${backHd("Prüfungsmodus", "Wie lief es?", "")}
   <div class="page">${p.liste.map(x => { const e = findEintrag(x.id), v = p.erg[x.id] || 0;
     return `<div class="rr"><div class="n">${e.nr ? e.nr + " · " : ""}${esc(e.title)}<small>${esc(e.angriff ? e.angriff.name + (e.angriff.zusatz ? " · " + e.angriff.zusatz : "") : e.gruppe.split("|")[0])}${x.stich ? " · Stichprobe " + esc(findGrad(e)) : ""}</small></div><span class="r4">${[1,2,3,4].map(k => `<button class="${k === v ? "on" : ""}" onclick="S.pr.erg['${x.id}']=${k};save();render()">${k}</button>`).join("")}</span></div>`; }).join("")}
    <p class="muted" style="padding:6px 16px">1 gesehen · 2 mit Hilfe · 3 sicher · 4 prüfungsreif. Übernehmen setzt den Stand der Techniken und speichert die Runde in der Statistik.</p>
    <div class="btnrow"><button class="btn" ${Object.keys(p.erg).length === p.liste.length ? "" : "disabled style='opacity:.4'"} onclick="prFertig()">Übernehmen</button></div>
   </div></div>`;
};
function prHist(){
  if (!S.prHist) { S.prHist = []; for (let k = 6; k >= 1; k--) S.prHist.push({ d: +heute0() - k * 9 * TAG, disc: "aikido", grade: "k5", n: 14, schnitt: 2 + (6 - k) * 0.3 + hash("p" + k) * 0.3 }); }
  return S.prHist;
}
function prFertig(){
  const v = Object.values(S.pr.erg);
  prHist().push({ d: +heute0(), disc: S.disc, grade: S.grade[S.disc], n: v.length, schnitt: v.reduce((a, b) => a + b, 0) / v.length });
  for (const [id, r] of Object.entries(S.pr.erg)) { S.ratings[id] = r; logge(id); }
  save(); S.stack = [["menu"], ["statistik"]]; render(); toast("Stand gesetzt, Runde in der Statistik");
}

/* ---------- Statistik ---------- */
V.statistik = () => {
  const d = disc(S.disc), segs = [["aikido","Aikido"],["daitoryu","Daitoryu"],["shinkitoho","Shinki Toho"]];
  const grade = d.grade.filter(x => uebbar(eintraege(S.disc, x.id)).length);
  const runden = prHist().filter(h => h.disc === S.disc).slice(-8);
  let tage = 0, tech = 0; for (let k = 0; k < 30; k++) { const n = geuebtAm(isoD(new Date(heute0() - k * TAG))); if (n) { tage++; tech += n; } }
  return `${sbar()}<div class="body" data-scroll="stat"><div class="ink"></div>${backHd("Menü", "Statistik", "記録")}
   <div class="page">
    <div class="grid2">
     <div class="card2"><h3>Letzte 30 Tage</h3><div class="row2"><span><span class="big1">${tage}</span><br><span class="muted">Übungstage</span></span><span><span class="big1">${tech}</span><br><span class="muted">Techniken geübt</span></span></div></div>
     <div class="card2"><h3>Übungstage <span>26 Wochen · <a href="#" style="color:var(--pri)" onclick="go('jahre');return false">alle Jahre ›</a></span></h3>${heatmap(26)}</div>
    </div>
    <div class="seg">${segs.map(([id,t]) => `<button class="${id === S.disc ? "on" : ""}" onclick="S.disc='${id}';save();render()">${t}</button>`).join("")}</div>
    <div class="grid2">
     <div class="card2"><h3>Stand je Grad <span>Anzahl Techniken</span></h3>
      ${grade.map(x => { const ids = uebbar(eintraege(S.disc, x.id)).map(e => e.id), n = [0,0,0,0,0]; ids.forEach(i => n[rating(i)]++);
        return `<div class="gbar"><span>${esc(x.name)}</span><div class="bar">${[4,3,2,1].map(r => n[r] ? `<i style="width:${n[r] / ids.length * 100}%;background:var(--r${r})" data-tip="${esc(x.name)}: ${n[r]} ${RATING[r]}"></i>` : "").join("")}</div><b>${n[4]}/${ids.length}</b></div>`; }).join("")}
      <div style="margin-top:8px">${legend()}</div><p>Rechts: prüfungsreif von allen Techniken des Grades.</p></div>
     <div class="card2"><h3>Prüfungsmodus <span>Schnitt je Runde</span></h3>
      ${runden.length ? `<div class="cols"><div class="yax"><span>4</span><span>3</span><span>2</span><span>1</span></div>
       <div class="plot">${[1,2,3].map(v => `<div class="gl" style="bottom:${(v - 1) / 3 * 100}%"></div>`).join("")}${runden.map(h => `<div class="c" style="height:${(h.schnitt - 1) / 3 * 100}%" data-tip="${kurzD(new Date(h.d))} · ${esc(d.grade.find(x => x.id === h.grade)?.name || "")} · ${h.n} Techniken · Schnitt ${h.schnitt.toFixed(1).replace(".", ",")}"><span>${new Date(h.d).toLocaleDateString("de-DE", { day: "numeric", month: "numeric" })}</span></div>`).join("")}</div></div>
       <p>Senkrecht: Durchschnitt der Einschätzungen (1 gesehen … 4 prüfungsreif). Waagerecht: Datum der Runde.</p>` : `<p>Noch keine Runde im Prüfungsmodus.</p>`}</div>
    </div>
    <div class="card2"><h3>Meine Grade <span><a href="#" style="color:var(--pri)" onclick="go('grade');return false">bearbeiten ›</a></span></h3>
     ${S.meineGrade[S.disc].length ? S.meineGrade[S.disc].slice().sort((a, b) => a.d.localeCompare(b.d)).map(m => `<div class="row2" style="padding:2px 0"><span>${esc(gradName(S.disc, m.g))}</span><span class="muted">${parseD(m.d).toLocaleDateString("de-DE")} · ${esc(m.ort || "")}</span></div>`).join("") : `<p>Noch kein Grad eingetragen.</p>`}
     ${naechstePruefung(S.disc) ? `<p>Vorgemerkt: ${esc(naechstePruefung(S.disc).text)}, ${naechstePruefung(S.disc).von.toLocaleDateString("de-DE")} in ${esc(naechstePruefung(S.disc).ort)}</p>` : ""}</div>
   </div></div>`;
};

/* ---------- Lehrgänge ---------- */
function kurse(){
  if (!S.kurse) {
    const pf = LE.find(x => x.ort === "Pforzheim" && x.von.startsWith("2026-10")) || LE[0];
    const kote = D.disziplinen[0].techniken.find(t => t.name.startsWith("Kote-gaeshi"));
    S.kurse = { k1: Object.assign({ key: "k1", eintraege: [
      { tm: "Sa 11:15", text: "Meditation: Atem bis in den Hara, Ausatmen doppelt so lang wie Einatmen.", tags: {} },
      { tm: "Sa 11:40", text: "Kote-gaeshi aus Tsuki: erst ausweichen, dann den Arm führen, nicht reißen.", tags: { angriff: "Tsuki", technik: "Kote-gaeshi" }, link: kote && kote.id },
      { tm: "Sa 15:30", text: "Fußstellung beim Tenkan, Skizze.", tags: { technik: "Tenkan" }, foto: true },
      { tm: "So 10:20", text: "Pausengespräch mit Soke über Zanshin.", tags: {}, audio: "1:12" }
    ] }, pf) };
  }
  return S.kurse;
}
const kursRange = k => { const v = parseD(k.von), b = k.bis ? new Date(parseD(k.bis) - TAG) : v; return +b > +v ? `${v.getDate()}.${b.getMonth() !== v.getMonth() ? (v.getMonth() + 1) + "." : ""}–${b.getDate()}.${b.getMonth() + 1}.${b.getFullYear()}` : `${v.getDate()}.${v.getMonth() + 1}.${v.getFullYear()}`; };
const lehrer = k => (k.info || "").match(/^mit (.+?)(?= Samstag| Freitag| Montag| Sonntag|$)/i)?.[1] || k.lehrer || "";
function kursAus(i){ const x = LE[i], key = "e" + i; const ks = kurse(); if (!ks[key]) ks[key] = Object.assign({ key, eintraege: [] }, x); save(); return key; }
function evRow(x, i, on){
  const v = parseD(x.von), tage = Math.round((v - heute0()) / TAG), k = Object.values(kurse()).find(k => k.uid && k.uid === x.uid);
  return `<div class="ev ${on ? "on" : ""}" onclick="${k ? `kursOeffnen('${k.key}')` : `kursOeffnen(kursAus(${i}))`}"><div class="dt"><b>${v.getDate()}</b><span>${v.toLocaleDateString("de-DE", { month: "short" })}</span></div>
    <div class="t">${esc(x.titel)}<small>${esc(x.ort || "Ort offen")} · ${kursRange(x)}${lehrer(x) ? " · " + esc(lehrer(x)) : ""}</small></div><em>${tage >= 0 ? `in ${tage} T.` : ""}${k ? `<br>${k.eintraege.length} Notizen` : ""}</em>${k && k.pruefung ? `<div style="grid-column:2/-1">${pruefChip(k)}</div>` : ""}</div>`;
}
function kursOeffnen(key){ if (T()) { S.kursSel = key; save(); render(); } else go("lehrgang", key); }
/* Meine Grade und geplante Prüfungen: Grundlage für Wartezeiten, Hinweise und Statistik */
S.meineGrade = S.meineGrade || { aikido: [{ g: "k5", d: "2025-06-14", ort: "Weimar" }], daitoryu: [], shinkitoho: [{ g: "shoden", d: "2025-09-14", ort: "Königheim" }] };
const gradName = (did, g) => (disc(did).grade.find(x => x.id === g) || {}).name || g;
const discName = did => DISC.find(x => x[0] === did)[1];
function hoechster(did){ const l = S.meineGrade[did], o = disc(did).grade.map(x => x.id); return l.slice().sort((a, b) => o.indexOf(b.g) - o.indexOf(a.g))[0]; }
function naechsterGrad(did){ const o = disc(did).grade.map(x => x.id), h = hoechster(did); return o[h ? o.indexOf(h.g) + 1 : 0]; }
/* Frühester Termin für einen Grad: nur wo die Daten einen Mindestabstand nennen (Shinki Toho) */
function fruehestens(did, g){
  const o = disc(did).grade, i = o.findIndex(x => x.id === g), j = (o[i].abstand || "").match(/(\d+) Jahr/);
  const vor = i > 0 && S.meineGrade[did].find(x => x.g === o[i - 1].id);
  if (!j || !vor) return null;
  const d = parseD(vor.d); d.setFullYear(d.getFullYear() + +j[1]); return d;
}
function naechstePruefung(did){
  const l = Object.values(kurse()).filter(k => k.pruefung && parseD(k.von) >= heute0() && (!did || k.pruefung.disc === did)).sort((a, b) => a.von.localeCompare(b.von));
  if (!l.length) return null;
  const k = l[0], p = k.pruefung;
  return { text: `${discName(p.disc)} ${gradName(p.disc, p.grade)}`, tage: Math.round((parseD(k.von) - heute0()) / TAG), ort: k.ort || k.titel, disc: p.disc, grade: p.grade, key: k.key, von: parseD(k.von) };
}
function wartezeitText(did, g, am){
  const f = fruehestens(did, g);
  if (!f) { const o = disc(did).grade, i = o.findIndex(x => x.id === g); return i <= 0 ? "erster Grad, keine Wartezeit" : `${o[i].abstand || "Wartezeit"} nach ${o[i - 1].name}: vorherigen Grad eintragen`; }
  const fmt2 = f.toLocaleDateString("de-DE", { day: "numeric", month: "numeric", year: "numeric" });
  return am ? (am >= f ? `passt (frühestens ${fmt2})` : `zu früh: frühestens ${fmt2}`) : `frühestens ${fmt2}`;
}
let PS = null; // Auswahl im Blatt „Prüfung hier ablegen“
function pruefBlatt(key){
  const k = kurse()[key]; PS = PS && PS.key === key ? PS : { key, disc: k.pruefung?.disc || S.disc, grade: k.pruefung?.grade || null };
  const did = PS.disc, ng = naechsterGrad(did), o = disc(did).grade.filter(x => x.id !== "hoshu");
  const g = PS.grade || ng;
  closeSheet();
  const el = document.createElement("div");
  el.className = "dim"; el.onclick = ev => { if (ev.target === el) { el.remove(); PS = null; } };
  el.innerHTML = `<div class="sheet"><div class="grab"></div><div><div class="k">${esc(k.titel)} · ${kursRange(k)}</div><h2 style="font-size:22px">Prüfung hier ablegen</h2></div>
    <div class="seg" style="margin:0">${DISC.map(([id, t]) => `<button class="${id === did ? "on" : ""}" onclick="PS.disc='${id}';PS.grade=null;pruefBlatt('${key}')">${t}</button>`).join("")}</div>
    <div class="grades" style="padding:0">${o.map(x => `<button class="gchip ${x.id === g ? "on" : ""}" onclick="PS.grade='${x.id}';pruefBlatt('${key}')"><b>${esc(x.name)}</b><span>${x.id === ng ? "nächster" : S.meineGrade[did].some(m => m.g === x.id) ? "abgelegt" : ""}</span></button>`).join("")}</div>
    <div class="meta">Wartezeit: ${esc(wartezeitText(did, g, parseD(k.von)))}</div>
    <div class="btns"><button class="btn" onclick="kurse()['${key}'].pruefung={disc:'${did}',grade:'${g}'};PS=null;save();closeSheet();render()">Vormerken</button><button class="btn ghost" onclick="delete kurse()['${key}'].pruefung;PS=null;save();closeSheet();render()">Keine Prüfung</button></div></div>`;
  $("#scr").appendChild(el);
}
const pruefChip = k => k.pruefung ? `<span class="chip lnk" style="background:var(--pri);color:#fff">Prüfung ${esc(discName(k.pruefung.disc))} ${esc(gradName(k.pruefung.disc, k.pruefung.grade))}</span>` : "";
V.grade = () => `${sbar()}<div class="body" data-scroll="grade"><div class="ink"></div>${backHd("Zurück", "Meine Grade", "段級")}
  <div class="page"><div class="grid2">${DISC.map(([did, t]) => { const l = S.meineGrade[did].slice().sort((a, b) => b.d.localeCompare(a.d)), ng = naechsterGrad(did), ex = naechstePruefung(did);
   return `<div class="card2"><h3>${t} <span>${l.length ? "zuletzt " + esc(gradName(did, l[0].g)) : "noch kein Grad"}</span></h3>
    ${l.map((m, i) => `<div class="row2" style="padding:4px 0;border-bottom:1px solid var(--bg2)"><span><b>${esc(gradName(did, m.g))}</b> <span class="muted">${parseD(m.d).toLocaleDateString("de-DE")} · ${esc(m.ort || "")}</span></span><button class="iconbtn" style="width:28px;height:28px;font-size:14px" title="Entfernen" onclick="S.meineGrade['${did}'].splice(S.meineGrade['${did}'].indexOf(S.meineGrade['${did}'].find(x=>x.g==='${m.g}')),1);save();render()">×</button></div>`).join("")}
    ${ng ? `<p>Nächster: <b style="color:var(--fg)">${esc(gradName(did, ng))}</b> · ${esc(wartezeitText(did, ng))}${ex ? ` · vorgemerkt: ${esc(ex.ort)}, ${ex.von.toLocaleDateString("de-DE")}` : ""}</p>` : ""}
    <div style="display:grid;grid-template-columns:1fr 1fr 1fr auto;gap:4px;margin-top:6px"><select id="g-${did}" style="border:1px solid var(--line2);background:var(--bg2);padding:6px">${disc(did).grade.filter(x => x.id !== "hoshu").map(x => `<option value="${x.id}" ${x.id === ng ? "selected" : ""}>${esc(x.name)}</option>`).join("")}</select><input id="d-${did}" type="date" value="${isoD(heute0())}" style="border:1px solid var(--line2);background:var(--bg2);padding:5px;min-width:0"><input id="o-${did}" placeholder="Ort" style="border:1px solid var(--line2);background:var(--bg2);padding:6px;min-width:0"><button class="btn small" style="width:auto" onclick="gradNeu('${did}')">+</button></div></div>`; }).join("")}</div>
  <p class="muted" style="padding:0 16px">Grundlage für Wartezeiten, die Hinweise vor einer Prüfung und die Statistik.</p></div></div>`;
function gradNeu(did){ const g = $("#g-" + did).value; S.meineGrade[did] = S.meineGrade[did].filter(x => x.g !== g); S.meineGrade[did].push({ g, d: $("#d-" + did).value || isoD(heute0()), ort: $("#o-" + did).value }); save(); render(); }
function lehrgangsListe(){
  const t = heute0(), kommend = LE.map((x, i) => [x, i]).filter(([x]) => parseD(x.bis || x.von) >= t).slice(0, 6);
  const eigene = Object.values(kurse()).sort((a, b) => b.von.localeCompare(a.von));
  const geplant = Object.values(kurse()).filter(k => k.pruefung && parseD(k.von) >= t).sort((a, b) => a.von.localeCompare(b.von));
  return `<div class="navrow" onclick="go('grade')"><span class="ic">段</span><div><b>Meine Grade</b><span>${DISC.map(([did, n]) => { const h = hoechster(did); return h ? `${n} ${gradName(did, h.g)}` : null; }).filter(Boolean).join(" · ") || "noch keine eingetragen"}</span></div><em>›</em></div>
    ${geplant.length ? `<div class="card2"><h3>Geplante Prüfungen</h3>${geplant.map(k => `<div class="row2" style="padding:3px 0;cursor:pointer" onclick="kursOeffnen('${k.key}')"><span><b>${esc(discName(k.pruefung.disc))} ${esc(gradName(k.pruefung.disc, k.pruefung.grade))}</b> <span class="muted">${esc(k.ort || k.titel)}, ${parseD(k.von).toLocaleDateString("de-DE")}</span></span><span class="muted">${esc(wartezeitText(k.pruefung.disc, k.pruefung.grade, parseD(k.von)))}</span></div>`).join("")}</div>` : ""}
    <div class="card2"><h3>Nächste Termine <span>Kalender shinkiryu</span></h3><p>Aus dem öffentlichen Lehrgangskalender (ICS). Tippen öffnet das Notizbuch; dort lässt sich eine Prüfung vormerken.</p></div>
    ${kommend.map(([x, i]) => evRow(x, i, T() && S.kursSel && kurse()[S.kursSel]?.uid === x.uid)).join("")}
    <div class="card2" style="margin-top:16px"><h3>Meine Notizbücher</h3></div>
    ${eigene.map(k => `<div class="ev ${T() && S.kursSel === k.key ? "on" : ""}" onclick="kursOeffnen('${k.key}')"><div class="dt"><b>${parseD(k.von).getDate()}</b><span>${parseD(k.von).toLocaleDateString("de-DE", { month: "short" })}</span></div><div class="t">${esc(k.titel)}<small>${esc(k.ort || "")} · ${k.eintraege.length} Notizen</small>${pruefChip(k)}</div><em>›</em></div>`).join("")}
    <div class="btnrow" style="margin-top:12px"><button class="btn ghost" onclick="go('lehrgangneu')">+ Lehrgang ohne Kalendereintrag</button></div>`;
}
V.lehrgaenge = () => {
  if (T()) { const sel = S.kursSel && kurse()[S.kursSel] ? S.kursSel : "k1";
    return `${sbar()}<div class="split2"><div class="l body" data-scroll="lg"><div class="ink"></div>${backHd("Menü", "Lehrgänge", "講習会")}${lehrgangsListe()}</div><div class="r body" data-scroll="kurs" style="display:flex;flex-direction:column">${kursInhalt(sel)}</div></div>`; }
  return `${sbar()}<div class="body" data-scroll="lg"><div class="ink"></div>${backHd("Menü", "Lehrgänge", "講習会")}<div class="page">${lehrgangsListe()}</div></div>`;
};
V.lehrgang = key => `${sbar()}<div class="body" data-scroll="kurs" style="display:flex;flex-direction:column"><div class="ink"></div>${backHd("Lehrgänge", "Notizbuch", "", `<button class="iconbtn" title="Exportieren" onclick="exportKurs('${key}')">⇪</button>`)}${kursInhalt(key)}</div>`;
/* Link der offiziellen Ausschreibung: Zeile „Ausschreibung: URL“ in der Beschreibung des Termins */
const ausschreibung = k => (k.info || "").match(/Ausschreibung:\s*(https?:\/\/\S+)/)?.[1] || "";
let STRUKTUR = false;
function kursInhalt(key){
  const k = kurse()[key]; if (!k) return "";
  const gl = D.glossar.woerter.map(w => w.wort);
  return `<div class="page" style="flex:1">
    <div class="card2"><div class="k" style="font:700 11px var(--f-cond);letter-spacing:.1em;text-transform:uppercase;color:var(--pri)">${kursRange(k)}${k.uid ? " · aus dem Kalender" : " · eigener Eintrag"}</div>
     <div style="font:600 21px/1.15 var(--f-serif);margin:3px 0">${esc(k.titel)}</div><p>${esc(k.ort || "")}${lehrer(k) ? " · " + esc(lehrer(k)) : ""}</p>
     ${k.pruefung ? `<div class="row2" style="margin-top:6px"><span>${pruefChip(k)} <span class="muted">${esc(wartezeitText(k.pruefung.disc, k.pruefung.grade, parseD(k.von)))}</span></span><b style="color:var(--pri);cursor:pointer;font-size:12.5px" onclick="pruefBlatt('${key}')">ändern</b></div>`
       : parseD(k.bis || k.von) >= heute0() ? `<button class="btn ghost small" style="width:auto;margin-top:6px" onclick="pruefBlatt('${key}')">Prüfung hier ablegen …</button>` : ""}
     ${k.uid ? `<button class="btn ghost small" style="width:auto;margin-top:6px" onclick="toast('Kalender des Geräts: Termin mit Zeit und Ort übernommen')">In den Kalender</button>` : ""}
     ${ausschreibung(k) ? `<button class="btn ghost small" style="width:auto;margin-top:6px" onclick="window.open('${esc(ausschreibung(k))}','_blank')">Ausschreibung ↗</button>` : ""}
     ${T() ? `<button class="btn ghost small" style="width:auto;margin-top:6px" onclick="exportKurs('${key}')">⇪ Exportieren</button>` : ""}</div>
    ${k.eintraege.map(en => `<div class="entry"><span class="tm">${esc(en.tm)}</span><p>${esc(en.text)}</p>
      ${en.foto ? `<div class="foto">Foto / Skizze</div>` : ""}${en.audio ? `<span class="audio">▶ <i></i> ${esc(en.audio)}</span>` : ""}
      ${Object.values(en.tags || {}).length || en.link ? `<div class="chips">${Object.entries(en.tags || {}).map(([a, v]) => `<span class="chip">${a === "angriff" ? "Angriff" : a === "technik" ? "Technik" : "Waffe"}: ${esc(v)}</span>`).join("")}${en.link ? `<span class="chip lnk" onclick="sheet('${en.link}')">↗ im Programm</span>` : ""}</div>` : ""}</div>`).join("") || `<p class="muted" style="padding:6px 16px">Noch keine Einträge. Unten schreiben, fotografieren oder sprechen.</p>`}
   </div>
   <div class="compose">
    ${STRUKTUR ? `<div class="tags"><input id="tA" list="gl" placeholder="Angriff"><input id="tT" list="gl" placeholder="Technik"><input id="tW" list="gl" placeholder="Waffe"></div><datalist id="gl">${gl.map(g => `<option value="${esc(g)}">`).join("")}</datalist>` : ""}
    <div class="rowc"><button class="ib ${STRUKTUR ? "on" : ""}" title="Angriff, Technik, Waffe" onclick="STRUKTUR=!STRUKTUR;render()">⌗</button><button class="ib" title="Foto" onclick="kursEintrag('${key}','foto')">📷</button><button class="ib" title="Sprachnotiz" onclick="kursEintrag('${key}','audio')">🎤</button>
     <input id="kt" placeholder="Was wurde gezeigt?" onkeydown="if(event.key==='Enter')kursEintrag('${key}')"><button class="send" onclick="kursEintrag('${key}')">+</button></div>
   </div>`;
}
function kursEintrag(key, art){
  const k = kurse()[key], txt = ($("#kt") || {}).value?.trim() || "", n = new Date();
  if (!txt && !art) return;
  const tags = {}; if (STRUKTUR) { [["angriff", "#tA"], ["technik", "#tT"], ["waffe", "#tW"]].forEach(([a, s]) => { const v = $(s)?.value.trim(); if (v) tags[a] = v; }); }
  const link = tags.technik && D.disziplinen.flatMap(d => d.techniken || []).find(t => t.name.toLowerCase() === tags.technik.toLowerCase() && (!tags.angriff || (t.angriff && t.angriff.name.toLowerCase() === tags.angriff.toLowerCase())));
  k.eintraege.push({ tm: n.toLocaleDateString("de-DE", { weekday: "short" }) + " " + n.toTimeString().slice(0, 5), text: txt || (art === "foto" ? "Foto" : "Sprachnotiz"), tags, foto: art === "foto", audio: art === "audio" ? "0:18" : null, link: link && link.id });
  save(); render();
  if (link) toast("Mit der Programmtechnik verknüpft; erscheint auch in ihren Notizen");
}
function exportKurs(key){
  const k = kurse()[key];
  const md = `# ${k.titel}\n${kursRange(k)} · ${k.ort || ""}${lehrer(k) ? " · " + lehrer(k) : ""}\n\n` + k.eintraege.map((en, i) => `**${en.tm}** ${en.text}${Object.values(en.tags || {}).length ? "  \n_" + Object.values(en.tags).join(" · ") + "_" : ""}${en.foto ? `  \n![Foto](bilder/${String(i + 1).padStart(2, "0")}-foto.jpg)` : ""}${en.audio ? `  \n[Sprachnotiz ${en.audio}](audio/${String(i + 1).padStart(2, "0")}-sprachnotiz.m4a)` : ""}`).join("\n\n") + "\n";
  const el = document.createElement("div");
  el.className = "dim"; el.onclick = ev => { if (ev.target === el) el.remove(); };
  el.innerHTML = `<div class="sheet"><div class="grab"></div><div><div class="k">Exportieren</div><h2 style="font-size:21px">${esc(k.titel)}</h2></div>
   <div class="muted">lehrgang-${k.von}.zip</div>
   <div class="md">notizbuch.md\n${k.eintraege.map((en, i) => en.foto ? `bilder/${String(i + 1).padStart(2, "0")}-foto.jpg\n` : en.audio ? `audio/${String(i + 1).padStart(2, "0")}-sprachnotiz.m4a\n` : "").join("")}\n── notizbuch.md ──\n${esc(md)}</div>
   <div class="btns"><button class="btn" onclick="closeSheet();toast('Teilen-Dialog des Systems (Mail, Messenger, Nextcloud …)')">Teilen …</button><button class="btn ghost" onclick="closeSheet();toast('Gespeichert als lehrgang-${k.von}.zip')">Als Datei</button></div>
   <span class="muted">Kleines Archiv: Markdown, Fotos und Sprachnotizen. Notizen bleiben sonst privat auf dem Gerät.</span></div>`;
  $("#scr").appendChild(el);
}
V.lehrgangneu = () => `${sbar()}<div class="body"><div class="ink"></div>${backHd("Lehrgänge", "Neuer Lehrgang", "")}
  <div class="page"><p class="muted" style="padding:0 16px 8px">Für private Trainings, Gastlehrer oder Termine, die nicht im Kalender stehen.</p>
   <div class="field"><label>Titel</label><input id="nT" value="Training mit Gastlehrer"></div>
   <div class="grid2" style="display:grid;grid-template-columns:1fr 1fr"><div class="field"><label>Von</label><input id="nV" type="date" value="${isoD(heute0())}"></div><div class="field"><label>Bis</label><input id="nB" type="date" value="${isoD(heute0())}"></div></div>
   <div class="field"><label>Ort</label><input id="nO" placeholder="Dojo, Stadt"></div>
   <div class="field"><label>Lehrer</label><input id="nL" placeholder="Name"></div>
   <div class="btnrow"><button class="btn" onclick="kursNeu()">Anlegen</button></div></div></div>`;
function kursNeu(){
  const ks = kurse(), key = "n" + Date.now().toString(36);
  const bis = new Date(parseD($("#nB").value || isoD(heute0())) - (-TAG));
  ks[key] = { key, titel: $("#nT").value || "Lehrgang", von: $("#nV").value || isoD(heute0()), bis: isoD(bis), ort: $("#nO").value, lehrer: $("#nL").value, info: "", eintraege: [] };
  save();
  if (T()) { S.kursSel = key; S.stack.pop(); }
  else S.stack[S.stack.length - 1] = ["lehrgang", key];
  save(); render();
}

/* ---------- Technik-Landkarte ---------- */
S.karte = S.karte || {};
V.karte = () => {
  const d = disc(S.disc), g = S.grade[S.disc], segs = [["aikido","Aikido"],["daitoryu","Daitoryu"],["shinkitoho","Shinki Toho"]];
  const order = d.grade.map(x => x.id), bis = order.slice(0, order.indexOf(g) + 1);
  let inhalt = "";
  if (S.disc === "shinkitoho") {
    const st = disc("shinkitoho"), fr = st.odachi;
    inhalt = `<div class="card2"><h3>Ô-dachi 1–43 <span>Stand je Form</span></h3></div><div class="formgrid">${fr.map(f => { const r = rating(f.id); return `<button style="background:var(--r${r});color:${r >= 2 ? "#fff" : "var(--fg)"}" data-tip="${f.nr} · ${esc(f.name)}: ${RATING[r]}" onclick="sheet('${f.id}')">${f.nr}</button>`; }).join("")}</div>`;
  } else {
    const gruppen = d.gruppen.filter(x => x.art === "angriffe");
    const gsel = gruppen.find(x => x.id === S.karte[S.disc]) || gruppen.find(x => d.techniken.some(t => t.gruppe === x.id && bis.includes(t.grad)));
    const ts = d.techniken.filter(t => t.gruppe === gsel.id && bis.includes(t.grad) && t.angriff);
    const ak = t => t.angriff.name + (t.angriff.zusatz ? " · " + t.angriff.zusatz : "");
    const cols = [...new Set(ts.map(ak))], rows = [...new Set(ts.map(t => t.name))];
    inhalt = `<div class="az" style="padding-top:8px">${gruppen.filter(x => d.techniken.some(t => t.gruppe === x.id && bis.includes(t.grad))).map(x => `<button style="${x.id === gsel.id ? "color:var(--fg);text-decoration:underline" : ""}" onclick="S.karte['${S.disc}']='${x.id}';save();render()">${esc(x.titel)}</button>`).join("")}</div>
     ${ts.length ? `<div class="mxw"><table class="mx"><thead><tr><th></th>${cols.map(c => `<th><div>${esc(c)}</div></th>`).join("")}</tr></thead><tbody>${rows.map(rn => `<tr><th>${esc(rn)}</th>${cols.map(c => { const hit = ts.filter(t => t.name === rn && ak(t) === c); if (!hit.length) return `<td class="no"></td>`; const r = Math.max(...hit.map(t => rating(t.id))); return `<td class="r${r}" data-tip="${esc(rn)} · ${esc(c)} · ${esc(d.grade.find(x => x.id === hit[0].grad).name)}: ${RATING[r]}" onclick="sheet('${hit[0].id}')">${r || ""}</td>`; }).join("")}</tr>`).join("")}</tbody></table></div>` : `<p class="muted" style="padding:0 16px">Keine Techniken in dieser Gruppe bis ${esc(d.grade.find(x => x.id === g).name)}.</p>`}`;
  }
  return `${sbar()}<div class="body" data-scroll="karte"><div class="ink"></div>${backHd("Menü", "Landkarte", "技の地図")}
   <div class="page">
    <div class="seg">${segs.map(([id,t]) => `<button class="${id === S.disc ? "on" : ""}" onclick="S.disc='${id}';save();render()">${t}</button>`).join("")}</div>
    <div class="grades">${d.grade.filter(x => x.id !== "hoshu").map(x => `<button class="gchip ${x.id === g ? "on" : ""}" onclick="S.grade[S.disc]='${x.id}';save();render()"><b>bis ${esc(x.name)}</b></button>`).join("")}</div>
    ${inhalt}
    <div class="card2"><div class="legend">${[0,1,2,3,4].map(r => `<span><i style="background:var(--r${r})"></i>${r} ${RATING[r]}</span>`).join("")}</div>
     <p>Zeilen: Technik, Spalten: Angriff; leer = kommt bis zum gewählten Grad nicht vor. Tippen öffnet das Blatt.</p></div>
   </div></div>`;
};

/* ---------- Glossar & Suche ---------- */
const norm = s => String(s || "").normalize("NFD").replace(/[̀-ͯ]/g, "").toLowerCase().replace(/[\s\-·'’]/g, "");
let SUCHE = "";
function suchErgebnisse(q){
  const n = norm(q), out = { Programm: [], Glossar: [], Notizen: [], Lehrgänge: [], Kommentare: [] };
  if (!n) return out;
  /* Kanji: Okurigana weglassen, damit 切り落とし auch 切り落し findet */
  const kana = s => String(s || "").replace(/[\u3040-\u309f\s・]/g, ""), qk = kana(q), cjk = /[\u4e00-\u9fff]/.test(q);
  const passt = (...xs) => xs.some(x => x && (norm(x).includes(n) || (cjk && qk && kana(x).includes(qk))));
  for (const d of D.disziplinen) for (const g of d.grade) for (const e of uebbar(eintraege(d.id || d.kurz, g.id))) {
    if (passt(e.title, e.jp, e.angriff?.name, e.angriff?.jp)) out.Programm.push({ id: e.id, t: (e.nr ? e.nr + " · " : "") + e.title, s: `${d.titel.replace(" Shinki Rengo", "")} · ${g.name}${e.angriff ? " · " + e.angriff.name : ""}`, jp: e.jp, open: `sheet('${e.id}')` });
    if (e.komm && norm(e.komm.text).includes(n) && out.Kommentare.length < 6) out.Kommentare.push({ t: e.komm.titel, s: "Kommentar shinkiryu.de", open: `go('kommentar','${e.id}')` });
  }
  for (const g of GLOSSAR) if (passt(g.t, g.jp, g.de)) out.Glossar.push({ t: g.t, jp: g.jp, s: g.de || "Glossar", say: true });
  for (const [id, l] of Object.entries(S.notes)) l.forEach(x => { if (passt(x)) out.Notizen.push({ t: x, s: findEintrag(id)?.title || "", open: `go('notizen','${id}')` }); });
  for (const k of Object.values(kurse())) k.eintraege.forEach(en => { if (passt(en.text, ...Object.values(en.tags || {}))) out.Lehrgänge.push({ t: en.text, s: `${k.titel} · ${en.tm}`, open: `kursOeffnen('${k.key}')` }); });
  out.Programm = out.Programm.slice(0, 30);
  return out;
}
/* Glossar: Aiki-Budo-Lexikon (shinkiryu.de) und Begriffe mit Kanji aus den Programmdaten, zusammengeführt von tools/daten.py */
const GLOSSAR = D.glossar.woerter.map(w => ({ t: w.wort, jp: w.jp || "", de: w.de }));
V.glossar = () => {
  const r = suchErgebnisse(SUCHE), hat = Object.values(r).some(x => x.length);
  const resRow = x => `<div class="res" ${x.open ? `onclick="${x.open}"` : ""}><div class="n">${esc(x.t)} ${x.jp ? `<span class="jp muted">${esc(x.jp)}</span>` : ""}<small>${esc(x.s)}</small></div>${x.say ? `<button class="say" title="Aussprache" onclick="event.stopPropagation();toast('Sprachschnipsel (Sprachsynthese): ${esc(x.t)}')">▶</button>` : "<span></span>"}</div>`;
  const buch = [...new Set(GLOSSAR.map(g => g.t[0].toUpperCase()))];
  return `${sbar()}<div class="body" data-scroll="gl"><div class="ink"></div>${backHd("Zurück", "Glossar & Suche", "辞書")}
   <div class="page">
    <div class="search"><input id="q" value="${esc(SUCHE)}" placeholder="Kiriotoshi, 切り落とし oder Hüftwurf" oninput="SUCHE=this.value;render();const i=document.getElementById('q');i.focus();i.setSelectionRange(i.value.length,i.value.length)"><span>⌕</span></div>
    ${SUCHE ? (hat ? Object.entries(r).filter(([, l]) => l.length).map(([k, l]) => `<div class="gh">${k}<span class="jp">${l.length}</span></div>${l.map(resRow).join("")}`).join("") : `<p class="muted" style="padding:0 16px">Nichts gefunden.</p>`)
     : `<p class="muted" style="padding:0 16px 6px">${GLOSSAR.length} Begriffe; Übersetzungen aus dem Aiki-Budo-Lexikon von shinkiryu.de (Stand 2018). Die Suche findet auch deutsche Wörter und durchsucht Programm, Kommentare, Notizen und Lehrgänge.</p>
       <div class="az">${buch.map(b => `<button onclick="document.getElementById('b-${b}').scrollIntoView()">${b}</button>`).join("")}</div>
       ${buch.map(b => `<div class="gh" id="b-${b}">${b}</div>${GLOSSAR.filter(g => g.t[0].toUpperCase() === b).map(g => resRow({ t: g.t, jp: g.jp, s: g.de || "Übersetzung fehlt im Lexikon", say: true })).join("")}`).join("")}`}
   </div></div>`;
};

/* ---------- Einstellungen, Videos, Code, Sicherung ---------- */
const BUNDLES = [
  { id: "aikido-kyu", t: "Aikido · alle Kyu", s: "5.–1. Kyu · 5 Videos", mb: 600, disc: "aikido", g: ["k5","k4","k3","k2","k1"] },
  { id: "aikido-dan", t: "Aikido · alle Dan", s: "1.–3. Dan · 3 Videos", mb: 170, disc: "aikido", g: ["d1","d2","d3"] },
  { id: "daitoryu-kyu", t: "Daitoryu · alle Kyu", s: "5.–1. Kyu · 5 Videos", mb: 330, disc: "daitoryu", g: ["k5","k4","k3","k2","k1"] },
  { id: "daitoryu-dan", t: "Daitoryu · alle Dan", s: "1.–3. Dan, Okugi · 4 Videos", mb: 270, disc: "daitoryu", g: ["d1","d2","d3","ok"] },
  { id: "st-shoden", t: "Shinki Toho · Suburi, Kamae, Shoden", s: "2 Videos", mb: 115, disc: "shinkitoho", g: ["shoden"] },
  { id: "st-chuden", t: "Shinki Toho · Chûden", s: "1 Video", mb: 67, disc: "shinkitoho", g: ["chuden"] },
  { id: "st-okuden", t: "Shinki Toho · Okuden", s: "noch nicht erschienen", ph: true }
];
const installiert = id => { const b = BUNDLES.find(x => x.id === id); return b && !b.ph && hasVideo(b.disc, b.g[0]); };
let LADEN = {};
function laden(id, ziel){
  LADEN[id] = 0;
  const t = setInterval(() => { LADEN[id] += 9; if (LADEN[id] >= 100) { clearInterval(t); delete LADEN[id]; if (ziel) S.bundleVer[id] = ziel; else freischalten(id); save(); toast(ziel ? "Neue Videoversion installiert" : "Videos geladen"); } if (["videos", "einst"].includes(S.stack.at(-1)[0])) render(); }, 160);
  render();
}
function freischalten(id){ S.bundles = S.bundles === "keine" && id === "aikido-kyu" ? "aikido-kyu" : "alle"; }

/* Sicherung: Datei, Android Auto Backup und echter Abgleich zwischen Geräten über WebDAV/Nextcloud mit Konfliktlösung */
S.konflikte = S.konflikte == null ? 2 : S.konflikte;
function sicherungSeite(){
  const sw = (k, label, sub) => `<div class="tgl"><span>${label}<small>${sub}</small></span><button class="sw ${S.sync[k] ? "on" : ""}" onclick="S.sync['${k}']=!S.sync['${k}'];save();render()"></button></div>`;
  return `<div class="card2"><h3>Datei</h3><p>Alles in einer Datei: Einschätzungen, Notizen, Lehrgänge mit Fotos und Sprachnotizen, Grade, Prüfungsrunden. Videos nicht (die lädt Mitori über den gespeicherten Code neu).</p>
     <div class="btns" style="margin-top:8px"><button class="btn" onclick="toast('Gespeichert: mitori-sicherung-${isoD(heute0())}.zip')">Exportieren …</button><button class="btn ghost" onclick="toast('Datei wählen; vorher zeigt Mitori, was ersetzt wird')">Importieren …</button></div></div>
    <div class="card2" style="padding:0"><h3 style="padding:12px 16px 0">Automatisch</h3>
     ${sw("backup", "Android Auto Backup", "ins Google-Konto, wenn das Gerät es anbietet · zuletzt gestern 03:12")}
     ${sw("webdav", "Abgleich über WebDAV / Nextcloud", "zwischen allen Geräten, auch Plasma Mobile")}
     ${S.sync.webdav ? `
      ${S.konflikte ? `<div class="notice" style="margin:10px 16px"><span>${S.konflikte} ${S.konflikte === 1 ? "Konflikt" : "Konflikte"}: derselbe Eintrag wurde auf zwei Geräten geändert.</span><b onclick="go('konflikt')">Lösen ›</b></div>` : ""}
      <div style="padding:6px 16px"><div class="row2" style="padding:4px 0"><span>📱 Telefon (dieses Gerät)</span><span class="muted">heute 18:40</span></div><div class="row2" style="padding:4px 0"><span>▭ Tablet am Mattenrand</span><span class="muted">heute 19:02</span></div></div>
      <div style="padding:6px 0 4px"><div class="field"><label>Server</label><input value="https://cloud.example.org/remote.php/dav/files/ich/"></div>
      <div class="grid2" style="display:grid;grid-template-columns:1fr 1fr"><div class="field"><label>Benutzer</label><input value="ich"></div><div class="field"><label>App-Passwort</label><input type="password" value="geheim-geheim"></div></div>
      <div class="field"><label>Ordner</label><input value="Mitori/"></div>
      ${sw("auto", "Nach jeder Änderung abgleichen", "sonst beim Öffnen und einmal am Tag")}
      <p class="muted" style="padding:6px 16px 0">Jede Einschätzung und jede Notiz ist ein eigener Eintrag mit Zeitstempel; Änderungen von verschiedenen Geräten werden zusammengeführt. Nachgefragt wird nur, wenn derselbe Eintrag auf zwei Geräten geändert wurde.</p>
      <div class="btnrow" style="padding:10px 16px"><button class="btn ghost small" onclick="toast('Abgeglichen: 3 Änderungen hoch, 5 herunter')">Jetzt abgleichen</button></div></div>` : ""}
    </div>`;
}
const KONFLIKTE = [
  { art: "Notiz", wo: "Shiho-nage · Katate-katate-dori (Gyaku-hanmi) · 29.10.", a: ["Telefon, 29.10. 21:14", "Hüfte früher drehen, Arm bleibt vor dem Kopf."], b: ["Tablet, 30.10. 18:02", "Hüfte früher drehen, Arm bleibt vor dem Kopf. Uke nicht nach unten ziehen."], beide: true },
  { art: "Einschätzung", wo: "Ikkyo · Shomen-uchi · heute", a: ["Telefon, heute 18:31", "3 · sicher"], b: ["Tablet, heute 18:55", "2 · mit Hilfe"], beide: false }
];
V.konflikt = () => {
  const offen = KONFLIKTE.slice(KONFLIKTE.length - S.konflikte);
  return `${sbar()}<div class="body"><div class="ink"></div>${backHd("Sicherung", "Konflikte", "")}
   <div class="page">${offen.length ? offen.map(k => `<div class="card2"><h3>${k.art} <span>${esc(k.wo)}</span></h3>
     ${[k.a, k.b].map((v, i) => `<div class="lastnote" style="margin:6px 0"><span>${esc(v[0])}</span>${esc(v[1])}</div>`).join("")}
     <div class="btns" style="margin-top:8px;grid-template-columns:${k.beide ? "1fr 1fr 1fr" : "1fr 1fr"}"><button class="btn ghost small" onclick="konfliktWeg()">Telefon</button><button class="btn ghost small" onclick="konfliktWeg()">Tablet</button>${k.beide ? `<button class="btn small" onclick="konfliktWeg()">Beide behalten</button>` : ""}</div></div>`).join("")
     : `<div class="card2"><p>Keine Konflikte. Alle Geräte sind auf dem gleichen Stand.</p></div>`}
    <p class="muted" style="padding:0 16px">Bei Notizen gibt es „Beide behalten“; bei Einschätzungen gilt eine Version, die andere bleibt im Verlauf sichtbar.</p></div></div>`;
};
function konfliktWeg(){ S.konflikte = Math.max(0, S.konflikte - 1); save(); render(); toast("Übernommen und auf allen Geräten abgeglichen"); }

/* Videos: Bundles einzeln entfernen und ohne Code neu laden; Download standardmäßig nur im WLAN */
S.entfernt = S.entfernt || {};
S.wlan = S.wlan == null ? true : S.wlan;
S.netz = S.netz || "wlan";
function entferntFuer(did, g){ return BUNDLES.some(b => S.entfernt[b.id] && b.disc === did && b.g.includes(g)); }
function ladenOk(id, ziel){ if (S.wlan && S.netz !== "wlan") { LADEN[id] = "wlan"; render(); return; } delete S.entfernt[id]; laden(id, ziel); }
function videosSeite(){
  const inst = BUNDLES.filter(b => installiert(b.id)), mb = inst.reduce((a, b) => a + b.mb, 0);
  return `<div class="card2"><h3>Speicher <span>${(mb / 1000).toFixed(2).replace(".", ",")} GB Videos</span></h3>
     <div class="store"><i style="flex:${mb || 1};background:var(--pri)" data-tip="Videos: ${mb} MB"></i><i style="flex:40;background:var(--r2)" data-tip="Fotos und Sprachnotizen: 40 MB"></i><i style="flex:${Math.max(200, 4000 - mb)};background:var(--r0)" data-tip="frei: ${((4000 - mb) / 1000).toFixed(1)} GB"></i></div>
     <div class="legend"><span><i style="background:var(--pri)"></i>Videos</span><span><i style="background:var(--r2)"></i>Fotos, Sprachnotizen</span><span><i style="background:var(--r0)"></i>frei</span></div></div>
    <div class="card2" style="padding:0"><div class="tgl" style="border:0"><span>Nur im WLAN laden<small>Videos sind 70–600 MB groß · Simulation: <a href="#" onclick="S.netz=S.netz==='wlan'?'mobil':'wlan';save();render();return false">${S.netz === "wlan" ? "WLAN" : "mobile Daten"}</a></small></span>${swb(S.wlan, "S.wlan=!S.wlan;save();render()")}</div></div>
    ${BUNDLES.map(b => { const weg = S.entfernt[b.id], ok = installiert(b.id), frei = !b.ph && (ok || weg || hasVideoRoh(b)), v = S.bundleVer[b.id] || 1, up = ok && S.upd.video && b.id === "aikido-kyu" && v < 2, l = LADEN[b.id];
      return `<div class="bd"><b>${esc(b.t)}</b><span class="muted">${esc(b.s)}${b.mb ? ` · ${b.mb} MB` : ""}${ok ? ` · Version ${v}` : ""}</span>
       ${up ? `<span class="muted" style="grid-column:1">v2: neu geschnitten ohne schwarze Balken, Kapitel genauer</span>` : ""}
       <div class="act">${b.ph ? `<span class="st no">kommt später</span>`
         : l === "wlan" ? `<span class="st up">wartet auf WLAN</span><button class="btn ghost small" style="width:auto" onclick="delete LADEN['${b.id}'];delete S.entfernt['${b.id}'];laden('${b.id}')">Trotzdem laden</button>`
         : l != null ? `<span class="st up">lädt ${l} %</span>`
         : up ? `<span class="st up">Update</span><button class="btn small" style="width:auto" onclick="ladenOk('${b.id}',2)">Aktualisieren</button>`
         : ok ? `<span class="st ok">geladen</span><button class="btn ghost small" style="width:auto" onclick="S.entfernt['${b.id}']=true;save();render();toast('Entfernt. Der Code bleibt gespeichert, neu laden geht ohne Code.')">Entfernen</button>`
         : weg ? `<span class="st no">entfernt · Code gespeichert</span><button class="btn small" style="width:auto" onclick="ladenOk('${b.id}')">Laden</button>`
         : `<span class="st no">nicht freigeschaltet</span><button class="btn ghost small" style="width:auto" onclick="go('code')">Code</button>`}</div>
       ${typeof l === "number" ? `<div class="prog2"><i style="width:${l}%"></i></div>` : ""}</div>`; }).join("")}
    <div class="card2" style="margin-top:14px"><p>Die Prüfungsvideos gibt es als DVD beim Shinkiryu-Verband; der beiliegende Code schaltet die passenden Videos frei.</p>
     <div class="btns" style="margin-top:8px"><button class="btn" onclick="go('code')">Code eingeben</button><a class="btn ghost" href="${KAUF}" target="_blank" rel="noopener" style="text-decoration:none">DVD bestellen</a></div></div>`;
}
const hasVideoRoh = b => { const e = S.entfernt[b.id]; delete S.entfernt[b.id]; const r = hasVideo(b.disc, b.g[0]); if (e) S.entfernt[b.id] = e; return r; };
function darstellungSeite(){
  return `<div class="card2"><h3>Theme</h3><div class="opts" style="display:flex;gap:14px;font-size:14px">${[["shinki", "Shinki"], ["kante", "Kante"]].map(([v, t]) => `<label><input type="radio" name="th" ${S.theme === v ? "checked" : ""} onchange="S.theme='${v}';save();render()"> ${t}</label>`).join("")}</div><p>Der Clickdummy zeigt nur Shinki.</p></div>
   <div class="card2"><h3>Sprache</h3><div class="opts" style="display:flex;gap:14px;font-size:14px"><label><input type="radio" name="sp" checked> Deutsch</label><label><input type="radio" name="sp"> English</label></div></div>
   <div class="card2"><h3>Über Mitori</h3><p>Version 1.0.0 · Programmdaten Stand 2026 · Kommentare: shinkiryu.de · Lehrgänge: öffentlicher Kalender</p>
    <div class="tgl" style="padding:8px 0 0;border:0"><span>Auf neue Version prüfen<small>einmal am Tag, lädt nichts selbst</small></span><button class="sw on"></button></div></div>`;
}
const EINST = [["videos", "映", "Videos", "Bundles, Updates, Code"], ["sicherung", "保", "Sicherung", "Datei, Auto Backup, Nextcloud"], ["darstellung", "色", "Darstellung", "Theme, Sprache, Über"]];
const EINST_F = { videos: videosSeite, sicherung: sicherungSeite, darstellung: darstellungSeite };
V.einst = () => {
  if (T()) { const sel = S.einstSel || "videos";
    return `${sbar()}<div class="split2"><div class="l body"><div class="ink"></div>${backHd("Menü", "Einstellungen", "設定")}${EINST.map(([k, ic, t, s]) => `<div class="navrow ${k === sel ? "on" : ""}" onclick="S.einstSel='${k}';save();render()"><span class="ic">${ic}</span><div><b>${t}</b><span>${s}</span></div><em>›</em></div>`).join("")}</div>
      <div class="r body" data-scroll="einst"><div class="page" style="padding-top:14px">${EINST_F[sel]()}</div></div></div>`; }
  return `${sbar()}<div class="body"><div class="ink"></div>${backHd("Menü", "Einstellungen", "設定")}<div class="page">${EINST.map(([k, ic, t, s]) => `<div class="navrow" onclick="go('${k}')"><span class="ic">${ic}</span><div><b>${t}</b><span>${s}</span></div><em>›</em></div>`).join("")}</div></div>`;
};
V.videos = () => T() ? (S.einstSel = "videos", S.stack[S.stack.length - 1] = ["einst"], V.einst()) : `${sbar()}<div class="body" data-scroll="vid"><div class="ink"></div>${backHd("Zurück", "Videos", "映像")}<div class="page">${videosSeite()}</div></div>`;
V.sicherung = () => T() ? (S.einstSel = "sicherung", S.stack[S.stack.length - 1] = ["einst"], V.einst()) : `${sbar()}<div class="body" data-scroll="si"><div class="ink"></div>${backHd("Zurück", "Sicherung", "")}<div class="page">${sicherungSeite()}</div></div>`;
V.darstellung = () => `${sbar()}<div class="body"><div class="ink"></div>${backHd("Zurück", "Darstellung", "")}<div class="page">${darstellungSeite()}</div></div>`;

/* Code eingeben: 20 Zeichen Crockford-Base32, Groß/klein und Bindestriche egal, Prüfzeichen fängt Tippfehler */
let CODE = { v: "", msg: null };
const BEISPIELCODE = "7KQ2MX9HDV3TAPRWE6YC";
V.code = () => `${sbar()}<div class="body"><div class="ink"></div>${backHd("Zurück", "Code eingeben", "")}
  <div class="page"><p class="muted" style="padding:0 16px 6px">Der Code liegt der DVD bei oder kam per Mail. Groß-/Kleinschreibung und Bindestriche sind egal.</p>
   <input class="codein" id="ci" maxlength="23" placeholder="XXXXX-XXXXX-XXXXX-XXXXX" value="${esc(CODE.v)}" oninput="CODE.v=this.value;CODE.msg=null">
   ${CODE.msg ? `<div class="msg ${CODE.msg[0]}">${CODE.msg[1]}</div>` : ""}
   <div class="btns" style="margin:8px 16px 0"><button class="btn" onclick="codePruefen()">Freischalten</button><button class="btn ghost" onclick="qrScan()">▣ QR-Code scannen</button></div>
   <p class="muted" style="padding:12px 16px">Zum Ausprobieren: <a href="#" onclick="CODE.v='7KQ2M-X9HDV-3TAPR-WE6YC';render();return false">7KQ2M-X9HDV-3TAPR-WE6YC</a> schaltet „Aikido · alle Kyu“ frei; andere Codes zeigen die Fehlermeldungen.</p>
   <div class="card2"><h3>Was passiert</h3><p>Mitori sucht den Code im Verzeichnis der Bundles, öffnet damit den Schlüssel und lädt die Videos herunter (WLAN empfohlen). Der Code bleibt nur auf dem Gerät.</p></div></div></div>`;
function qrScan(){
  const el = document.createElement("div");
  el.className = "dim"; el.style.alignItems = "stretch"; el.style.background = "#000";
  el.innerHTML = `<div style="flex:1;display:grid;place-items:center;color:#fff;text-align:center;font-size:13px"><div><div style="width:200px;height:200px;border:3px solid var(--pri);margin:0 auto 14px;background:repeating-linear-gradient(45deg,#222 0 8px,#2a2a2a 8px 16px)"></div>QR-Code vom Aufkleber der DVD in den Rahmen halten</div></div>`;
  $("#scr").appendChild(el);
  setTimeout(() => { el.remove(); CODE.v = "7KQ2M-X9HDV-3TAPR-WE6YC"; CODE.msg = ["ok", "Code erkannt. Freischalten tippen."]; render(); }, 1300);
}
function codePruefen(){
  const c = CODE.v.toUpperCase().replace(/[^0-9A-Z]/g, "").replace(/O/g, "0").replace(/[IL]/g, "1");
  if (c.length !== 20) CODE.msg = ["err", `Der Code hat 20 Zeichen, eingegeben sind ${c.length}.`];
  else if (c === BEISPIELCODE) { CODE.msg = ["ok", "Gültig: Aikido · alle Kyu (600 MB). Download läuft …"]; CODE.v = ""; render(); setTimeout(() => { S.stack[S.stack.length - 1] = ["videos"]; laden("aikido-kyu"); }, 900); return; }
  else if (hash(c) < .5) CODE.msg = ["err", "Das Prüfzeichen passt nicht. Vermutlich ein Tippfehler; bitte noch einmal vergleichen."];
  else CODE.msg = ["err", "Diesen Code kennt Mitori nicht, oder er wurde gesperrt. Bei Fragen an den Verband wenden."];
  render();
}

/* Große Heatmap über Jahre (aus der Statistik) */
const START_JAHR = 2019;
S.jahrDisc = S.jahrDisc || "alle";
function spartAm(iso){ const h = hash("s" + iso); return h < .55 ? "aikido" : h < .85 ? "daitoryu" : "shinkitoho"; }
function geuebtVon(iso, did){ const n = geuebtAm(iso); return !did || did === "alle" || iso === isoD(heute0()) ? n : spartAm(iso) === did ? n : 0; }
function jahrBlock(y, did){
  const t = heute0(), jan = new Date(y, 0, 1), off = (jan.getDay() + 6) % 7, start = new Date(y, 0, 1 - off);
  const wochen = Math.ceil((off + (y % 4 ? 365 : 366)) / 7);
  let tage = 0, tech = 0; const cells = [], mon = [];
  for (let w = 0; w < wochen; w++) { const d0 = new Date(+start + (w * 7 + 6) * TAG); mon.push(d0.getDate() <= 7 && d0.getFullYear() === y ? d0.toLocaleDateString("de-DE", { month: "short" }) : ""); }
  for (let wd = 0; wd < 7; wd++) {
    cells.push(`<em>${wd % 2 === 0 ? "MDMDFSS"[wd] : ""}</em>`);
    for (let w = 0; w < wochen; w++) {
      const d = new Date(+start + (w * 7 + wd) * TAG);
      if (d.getFullYear() !== y || d > t || d < new Date(START_JAHR, 8, 1)) { cells.push(`<i style="background:none"></i>`); continue; }
      const n = geuebtVon(isoD(d), did), st = n <= 0 ? 0 : n < 5 ? 1 : n < 9 ? 2 : n < 13 ? 3 : 4;
      if (n) { tage++; tech += n; }
      cells.push(`<i style="background:var(--r${st})" data-tip="${kurzD(d)} ${y}: ${n ? n + " Techniken" : "kein Training"}"></i>`);
    }
  }
  const grade = DISC.flatMap(([id, nm]) => S.meineGrade[id].filter(m => m.d.startsWith(String(y))).map(m => `${nm} ${gradName(id, m.g)} (${parseD(m.d).toLocaleDateString("de-DE", { day: "numeric", month: "numeric" })})`));
  return `<div class="card2"><h3>${y} <span>${tage} Übungstage · ${tech} Techniken</span></h3>
    <div class="months" style="--w:${wochen}"><span></span>${mon.map(m => `<span>${m}</span>`).join("")}</div>
    <div class="heat" style="--w:${wochen};gap:1px">${cells.join("")}</div>
    ${grade.length ? `<p>Grade: ${esc(grade.join(", "))}</p>` : ""}</div>`;
}
V.jahre = () => {
  const jahre = []; for (let y = heute0().getFullYear(); y >= START_JAHR; y--) jahre.push(y);
  return `${sbar()}<div class="body" data-scroll="jahre"><div class="ink"></div>${backHd("Statistik", "Übungstage", "稽古の記録")}
   <div class="page">
    <div class="chips" style="padding:0 16px 6px">${[["alle", "Alle"], ...DISC].map(([id, t]) => `<button class="pill ${S.jahrDisc === id ? "on" : ""}" onclick="S.jahrDisc='${id}';save();render()">${t}</button>`).join("")}</div>
    <div class="lg2" style="padding:0 16px;justify-content:flex-start">weniger <i style="background:var(--r0)"></i><i style="background:var(--r1)"></i><i style="background:var(--r2)"></i><i style="background:var(--r3)"></i><i style="background:var(--r4)"></i> mehr Techniken je Tag · Spalten: Wochen, Zeilen: Wochentage</div>
    ${jahre.map(y => jahrBlock(y, S.jahrDisc)).join("")}
   </div></div>`;
};

/* ---------- Notizen rechts: Zweck und Fragen der neuen Ansichten ---------- */
Object.assign(NOTES, {
  notizen: { titel: "Notizen je Technik und Übungstag", zweck: "Jede Notiz gehört zu einem Tag, neueste oben; unten schreiben, Foto (📷) oder Sprachnotiz (🎤) wie im Lehrgang. Lang drücken (im Browser: lang klicken oder Rechtsklick) öffnet Bearbeiten/Löschen. Verknüpfte Lehrgangseinträge erscheinen hier auch. ☰ öffnet alle Notizen.", entschieden: ["Bearbeiten und Löschen per langem Drücken.", "Fotos und Sprachnotizen auch hier, im selben Format wie im Lehrgang.", "Übersicht „Alle Notizen“."], fragen: [] },
  allenotizen: { titel: "Alle Notizen", zweck: "Alle Notizen über alle Techniken und Lehrgänge, nach Tag, mit Filter je Sparte oder nur Lehrgänge. Erreichbar über ✎ im Programm und ☰ in den Notizen einer Technik.", fragen: [] },
  heute: { titel: "Heute üben · Wiederholen nach Plan", entschieden: ["Eine Sparte je Einheit (Umschalter oben), bis zu ihrem im Programm gewählten Grad.", "Keine Minutenangabe.", "Abstände einstellbar (Karte unten).", "Keine Erinnerung."], zweck: "Oben wählst du die Sparte der heutigen Einheit. Fällig ist, was unsicher ist oder lange nicht geübt wurde (gesehen nach 2, mit Hilfe nach 4, sicher nach 14, prüfungsreif nach 35 Tagen), dazu zwei neue Techniken aus dem aktuellen Grad. Lehrgangsnotizen zählen nicht. „Training starten“ geht die Liste durch: ansehen, einschätzen, weiter.", fragen: [] },
  session: { titel: "Training (Playliste)", zweck: "„Training starten“ erzeugt eine Playliste nur aus den heutigen Techniken. Der Player spielt die aktuelle; einschätzen schaltet zur nächsten offenen, vorige/nächste und ⤢ (Querformat) bleiben in der Playliste. Darunter (Tablet: links) die Playliste mit ✓ für Geübtes; Tippen springt hin. Ohne Video eine Karte mit Name und letzter Notiz.", entschieden: ["Playliste aus den heutigen Techniken."], fragen: [] },
  pruefung: { titel: "Prüfungsmodus · Einstellung", zweck: "Sparte und Grad wählen (eine vorgemerkte Prüfung wird angeboten), Anzahl, Reihenfolge und ob Techniken aus früheren Graden dazukommen. Shinki Toho: Shoden prüft Kamae (als ein Punkt) und Ô-dachi 1–11, die höheren Grade ihre Formen.", entschieden: ["Ergebnisse setzen auch den Stand der Techniken.", "Standard: Reihenfolge wie in der Prüfungsordnung; zufällig als Option.", "Techniken aus früheren Graden: Ja/Nein; 2–5 Stichproben, immer am Anfang.", "Kein Zeitdruck."], fragen: [] },
  runde: { titel: "Prüfungsmodus · Runde", zweck: "Tafel (Vorschlag D): Gruppe (Stand/Sitz), darunter „Angriff“ und „Technik“ beschriftet in zwei Zeilen. Bei Shinki Toho nur die Technik.", entschieden: ["Gestaltung D (Tafel).", "Shinki Toho: nur die Technik.", "Stichproben bleiben markiert."], fragen: [] },
  pruefende: { titel: "Prüfungsmodus · Ende", zweck: "Jede Technik einschätzen (1–4), dann in die Statistik übernehmen.", fragen: [] },
  statistik: { titel: "Statistik", zweck: "Kennzahlen der letzten 30 Tage, Übungstage als Heatmap (26 Wochen, „alle Jahre ›“ für die große Ansicht), Stand je Grad, Prüfungsrunden, Meine Grade. Jede Grafik mit Legende bzw. Achsen; Werte beim Überfahren.", entschieden: ["Weitere Auswertungen später (Todo)."], fragen: [] },
  lehrgaenge: { titel: "Lehrgänge", zweck: "Oben „Meine Grade“ und die geplanten Prüfungen mit Wartezeit-Prüfung. Nächste Termine kommen echt aus dem öffentlichen Kalender (ICS); im Notizbuch eines kommenden Lehrgangs „Prüfung hier ablegen …“ (Sparte + Grad). Die vorgemerkte Prüfung erscheint im Menü, in Heute üben, im Prüfungsmodus und in der Statistik. Tablet: Liste links, Notizbuch rechts.", entschieden: ["Abgelegte Grade eintragen (Meine Grade).", "Lehrgang markieren, auf dem eine Prüfung für eine Sparte geplant ist.", "Mindestabstände für alle Sparten aus den Programmdaten.", "Nur kommende Termine und vorhandene Notizbücher."], fragen: [] },
  grade: { titel: "Meine Grade", zweck: "Je Sparte die abgelegten Grade mit Datum und Ort, der nächste Grad mit frühestem Termin (wo die Daten einen Mindestabstand nennen) und eine vorgemerkte Prüfung.", fragen: [] },
  jahre: { titel: "Übungstage über Jahre", zweck: "Je Jahr eine Heatmap (Spalten Wochen, Zeilen Wochentage) mit Übungstagen und Techniken, Filter je Sparte, abgelegte Grade im Jahr darunter. Werte beim Überfahren.", fragen: [], entschieden: ["Große Übersicht reicht erstmal."] },
  konflikt: { titel: "Abgleich · Konflikte", zweck: "Nur wenn derselbe Eintrag auf zwei Geräten geändert wurde. Notiz: Telefon, Tablet oder beide behalten. Einschätzung: eine Version gilt, die andere bleibt im Verlauf.", fragen: [] },
  lehrgang: { titel: "Lehrgangs-Notizbuch", zweck: "Einträge mit Uhrzeit untereinander; unten schreiben, Foto (📷) oder Sprachnotiz (🎤). ⌗ blendet Angriff/Technik/Waffe mit Vorschlägen aus dem Glossar ein; passt das zu einer Programmtechnik, wird verknüpft und die Notiz erscheint auch dort. ⇪ exportiert als kleines ZIP-Archiv. Bei kommenden Lehrgängen: „Prüfung hier ablegen …“.", entschieden: ["Export als ZIP: Markdown mit Fotos und Sprachnotizen."], fragen: [] },
  lehrgangneu: { titel: "Lehrgang ohne Kalendereintrag", zweck: "Für private Trainings und Gastlehrer: Titel, Datum, Ort, Lehrer.", fragen: [] },
  karte: { titel: "Technik-Landkarte", zweck: "Je Gruppe (Tachi-waza Mae-seme …) ein Raster Technik × Angriff bis zum gewählten Grad, eingefärbt nach Stand; Shinki Toho als Raster der Ô-dachi-Formen. Tippen öffnet das Blatt, Überfahren zeigt Grad und Stand.", fragen: [], entschieden: ["Gruppen getrennt."] },
  glossar: { titel: "Glossar & Suche", zweck: "Ohne Eingabe: Glossar A–Z mit Kanji, Übersetzung und Aussprache-Knopf. Mit Eingabe: Treffer in Programm, Glossar (auch deutsche Wörter), Kommentaren, Notizen und Lehrgängen; „Kiriotoshi“, „Kiri-otoshi“ und 切り落とし finden dasselbe.", entschieden: ["Übersetzungen aus dem Aiki-Budo-Lexikon (tools/lexikon.py → daten/lexikon.yaml).", "Aussprache zunächst per Sprachsynthese, später vielleicht ersetzt."], fragen: [] },
  einst: { titel: "Einstellungen", zweck: "Videos (Bundles, Updates, Code), Sicherung (Datei, Android Auto Backup, WebDAV/Nextcloud), Darstellung (Theme, Sprache, Über). Tablet: Liste links, Seite rechts.", fragen: [] },
  videos: { titel: "Videos", zweck: "Speicher mit Legende, je Bundle Status und Version. Neue Bundle-Versionen haben einen eigenen Hinweis (hier und im Menü). „Entfernen“ gibt Speicher frei, der Code bleibt gespeichert, „Laden“ geht ohne Code. „Nur im WLAN laden“ ist an; mit mobilen Daten (Simulation umschaltbar) wartet der Download oder läuft mit „Trotzdem laden“.", entschieden: ["Bundles einzeln entfernbar, ohne Code neu ladbar.", "Download standardmäßig nur im WLAN."], fragen: [] },
  code: { titel: "Code eingeben", zweck: "20 Zeichen, Bindestriche und Groß-/Kleinschreibung egal, O/0 und I/L/1 werden gleich gelesen. Drei Fehlermeldungen: falsche Länge, Prüfzeichen (Tippfehler), unbekannt/gesperrt.", fragen: [], entschieden: ["Auch per QR-Code scanbar (Aufkleber auf der DVD)."] },
  sicherung: { titel: "Sicherung & Abgleich", zweck: "Export/Import als Datei, Android Auto Backup und echter Abgleich zwischen Geräten über WebDAV/Nextcloud. Einträge mit Zeitstempel werden zusammengeführt; Konflikte nur bei gleichzeitig geändertem Eintrag (Beispiel: 2 offene).", entschieden: ["Echter Abgleich mit Konfliktlösung."], fragen: [] },
  darstellung: { titel: "Darstellung", zweck: "Theme, Sprache, Version und Update-Prüfung.", fragen: [] }
});
for (const o of ORDER) o[2] = 1;
ORDER.splice(ORDER.findIndex(o => o[0] === "notizen"), ORDER.length, ["notizen","Notizen",1], ["allenotizen","Alle Notizen",1], ["heute","Heute üben",1], ["session","Training",1], ["pruefung","Prüfungsmodus",1], ["statistik","Statistik",1], ["lehrgaenge","Lehrgänge",1], ["lehrgang","Notizbuch",1], ["karte","Landkarte",1], ["glossar","Glossar & Suche",1], ["einst","Einstellungen",1], ["videos","Videos",1], ["code","Code",1], ["sicherung","Sicherung",1], ["grade","Meine Grade",1], ["jahre","Jahre",1], ["konflikt","Konflikte",1], ["runde","Prüfungsrunde",1]);
const K5 = "aikido/kyu-grade/tachi-waza-mae-seme/katate-katate-dori-gyaku-hanmi/k5/shiho-nage-1";
Object.assign(START, {
  notizen: [["menu"], ["programm"], ["notizen", K5]], allenotizen: [["menu"], ["programm"], ["allenotizen"]], session: [["menu"], ["heute"], ["session"]], heute: [["menu"], ["heute"]], pruefung: [["menu"], ["pruefung"]], statistik: [["menu"], ["statistik"]],
  lehrgaenge: [["menu"], ["lehrgaenge"]], lehrgang: [["menu"], ["lehrgaenge"], ["lehrgang", "k1"]], karte: [["menu"], ["karte"]], glossar: [["menu"], ["glossar"]],
  einst: [["menu"], ["einst"]], grade: [["menu"], ["lehrgaenge"], ["grade"]], jahre: [["menu"], ["statistik"], ["jahre"]], konflikt: [["menu"], ["einst"], ["sicherung"], ["konflikt"]],
  runde: [["menu"], ["pruefung"], ["runde"]], videos: [["menu"], ["einst"], ["videos"]], code: [["menu"], ["einst"], ["code"]], sicherung: [["menu"], ["einst"], ["sicherung"]]
});

/* Szenario: Update-Hinweise ein/aus (rechts unter „Szenario“) */
const _renderNotes = renderNotes;
renderNotes = function(){
  _renderNotes();
  const c = [...document.querySelectorAll("#notes .card")].find(x => x.querySelector("h3")?.textContent === "Szenario");
  if (c) c.querySelector(".opts").insertAdjacentHTML("afterend", `<div class="opts" style="margin-top:8px">${[["app", "Hinweis neue App-Version"], ["video", "Hinweis neue Videoversion"]].map(([k, t]) => `<label><input type="checkbox" ${S.upd[k] ? "checked" : ""} onchange="S.upd['${k}']=this.checked;save();render()">${t}</label>`).join("")}</div>`);
};

/* #heute usw. in der Adresse */
if (location.hash.length > 1 && START[location.hash.slice(1)]) S.stack = JSON.parse(JSON.stringify(START[location.hash.slice(1)]));
render();
