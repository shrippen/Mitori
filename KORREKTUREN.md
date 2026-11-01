# Korrekturen und offene Fragen

Befunde aus dem Abgleich der Vorlagen. Jeder Befund hat eine ID; dieselbe ID steht in
`daten/*.lua` am betroffenen Eintrag, solange er offen ist (Status *klären*), und erscheint
im Entwurf des Hefts (`./build.sh entwurf`) als rote Marke. Befunde mit Status *sicher* oder
*Schreibweise* sind in den Daten bereits umgesetzt und tragen keine Marke. Ist ein Befund geklärt: Daten ändern, ID dort
entfernen, hier den Status auf **erledigt** setzen.

**Status:** *sicher* = offensichtlicher Fehler, Vorschlag übernommen ·
*klären* = Vorlagen widersprechen sich, Entscheidung durch Sensei/Lehrer nötig ·
*Schreibweise* = nur Umschrift/Kanji-Variante, Inhalt gleich.

**Dokumente**

| Kürzel | Datei |
|---|---|
| PO | `quellen/aikido/Prüfungsordnung.pdf` (Stand 01.01.2022; S. 3 Kyu Mae-seme, S. 4 Kyu Ushiro/Suwari/Hanmi/Sonstige, S. 5–6 Dan) |
| KL | `quellen/aikido/Kyu-Techniken.doc` (Liste mit Kanji vom Sensei, 6 Seiten in LibreOffice) |
| DL | `quellen/aikido/Dan-Techniken.doc` (Liste mit Kanji vom Sensei, 3 Seiten) |
| DK | `quellen/daitoryu/Daitoryu_Prüfungsordnung_Kyu-Grade.pdf` (01.04.2008) |
| DD | `quellen/daitoryu/Daitoryu_Prüfungsordnung_Dan-Grade.pdf` (01.04.2008) |
| OT | `quellen/shinki-toho/shinkiryu.de/O-dachi Tabelle.pdf` (Stand 25.08.2010) |
| OK | `quellen/shinki-toho/Odachi Kommentare.pdf` (Übersicht S. 2–3, Formen ab S. 7) |
| V | Prüfungsvideos (Titelkarten, eingebrannte Beschriftungen, Kapitelmarken) |

Grundsatz im Heft: Bei Widerspruch gilt vorerst PO (Aikido) bzw. OT (Shinki Toho); die
Abweichung steht hier.

**Geprüft am 01.11.2026**, jede Zeile an den Quellen: PO, DK, DD, OT und OK über ihre Textebene
(`pdftotext -layout`), KL und DL über den Text aus LibreOffice, die Kanji der Formen an den
Titelkarten der Videos. Seitenangaben für KL/DL folgen der Seitenaufteilung in LibreOffice.
Ergebnis für Shinki Toho: Wo Tabelle und Kommentare sich widersprechen, weicht in jedem Fall nur
die Übersichtstabelle der Kommentare (OK S. 2–3) ab; OT und der Fließtext zu den Formen stimmen
überein.

---

## Aikido, Kyu-Grade

| ID | Dokument und Stelle | Bisher | Vorschlag | Begründung | Status |
|---|---|---|---|---|---|
| A-K-01 | PO S. 3, Zeile Kata, Spalte 2. Kyu; KL S. 1, Kata 2 Kyu | beide: „'I'achi-Suburi 2“ | Tachi-Suburi 2 | Tippfehler in beiden Vorlagen; Kanji in KL 太刀素振り２ | sicher |
| A-K-02 | PO S. 3, Katate-katate-dori (Ai-hanmi) 5. Kyu; KL S. 1, Katate-katate-dori (Gyaku-hanmi) 3 Kyu | „lkkyo“ (kleines L) | Ikkyo | Tippfehler; jeweils nur in einer der beiden Vorlagen | sicher |
| A-K-03 | PO S. 3, Gyaku-hanmi 2. Kyu Nikyo, 1. Kyu Sankyo/Yonkyo; KL S. 1, Spalte Erklärungen | PO: „(2 Arten)“; KL: „4 Arten 四種類“ | ? | Anzahl der Varianten widersprüchlich | klären |
| A-K-04 | PO S. 3, Ryote-ryote-dori 5. Kyu; KL S. 2 | Name „Shiho-gaeshi“, Kanji in KL 四方投げ (= Shiho-nage) | Name oder Kanji angleichen (四方返し?) | Shiho-nage kommt dort erst im 3. Kyu; Name und Kanji passen nicht zusammen | klären |
| A-K-05 | PO S. 3, Gyaku-hanmi 4. Kyu; KL S. 1 | PO: Ikkyo (soto-irimi), Kokyu-nage (ushiro / irimi); KL: Ikkyo soto-irimi/soto-tenkan, Kokyu-nage irimi | ? | Ausführungen weichen ab | klären |
| A-K-06 | PO S. 3, Gyaku-hanmi 1. Kyu Irimi-nage | „(soto-irimi 2 Arten)“ nur in PO | ? | KL nennt keine 2 Arten | klären |
| A-K-07 | PO S. 3, Yokomen-uchi 2. Kyu; KL S. 3 | PO: Yonkyo; KL zusätzlich Sankyo | ? | Sankyo nur in KL | klären |
| A-K-08 | PO S. 3, Tsuki 2. Kyu; KL S. 3 | PO: Yonkyo; KL zusätzlich Sankyo | ? | Sankyo nur in KL | klären |
| A-K-09 | PO S. 3, Tsuki 1. Kyu; KL S. 3, Tsuki 1 kyu | PO: Kiri-otoshi (soto-tenkan), Uchi-kaiten-nage (soto-tenkan); KL: Uchi-sankyo (uchi-tenkan u. anschließend soto-tenkan), Kiri-otoshi (ohne Zusatz), Uchi-kaiten-nage (soto-tenkan) | ? | Uchi-sankyo fehlt in PO; der Zusatz „soto-tenkan“ hängt in PO an Kiri-otoshi, in KL an Uchi-sankyo (verrutscht?) | klären |
| A-K-10 | KL S. 2, Mune-ude-dori 1. Kyu | Kiri-otoshi nur in KL | ? | fehlt in PO (dort nur 2. Kyu Hiji-gaeshi) | klären |
| A-K-11 | PO S. 4, Ushiro-seme; KL S. 4 | PO: „Ryote-kubi-jime“; KL: „Kubi-jime“, dazu 2. Kyu Ikkyo | ? | Angriffsname und Inhalt abweichend; im Heft ohne Kanji, bis geklärt | klären |
| A-K-12 | PO S. 4, Ushiro Ryote-dori 2. Kyu | PO: Yonkyo (auch im 1. Kyu, also doppelt); KL: Sankyo | 2. Kyu Sankyo? | Doppelnennung wirkt wie Fehler | klären |
| A-K-13 | PO S. 4, Ushiro Ryote-dori 2. Kyu Kokyu-nage; KL S. 4 | PO: „(2 Arten)“; KL: „eine Hand hoch“, Kanji 呼吸法 | ? | Ausführung und Kanji (呼吸法 = Kokyu-ho) abweichend | klären |
| A-K-14 | KL S. 4, Eri-dori 2 kyu | KL: „Irimi-nage (2)“; PO S. 4: „Irimi-nage“ | ? | Bedeutung von „(2)“ unklar (2 Arten?) | klären |
| A-K-15 | KL S. 4, Ryo-kata-dori 2. Kyu | Sankyo, Yonkyo nur in KL | ? | fehlen in PO | klären |
| A-K-16 | PO S. 4, Suwari-waza Katate-katate-dori diagonal; KL S. 4 | KL zusätzlich 2. Kyu Yonkyo, 1. Kyu Kokyu-nage | ? | Im Heft ist die ganze Gruppe Suwari-waza Mae-seme mit A-K-16 markiert | klären |
| A-K-17 | KL S. 4/5, Suwari Katate-katate-dori gerade | KL zusätzlich 2. Kyu Sankyo, Yonkyo | ? | fehlen in PO | klären |
| A-K-18 | KL S. 5, Suwari Ryote-dori | KL zusätzlich 3. Kyu Nikyo, 2. Kyu Sankyo, Yonkyo | ? | fehlen in PO | klären |
| A-K-19 | KL S. 5, Suwari Katate-kata-dori | KL zusätzlich 2. Kyu Sankyo, Yonkyo; Kanji 片手片手取り | Kanji 片手肩取り? | Kanji passt nicht zum Namen (kata = 肩) | klären |
| A-K-20 | KL S. 5, Suwari Shomen-uchi | KL zusätzlich 2. Kyu Sankyo; 4. Kyu Ikkyo ohne Kanji | ? | Abweichung PO/KL | klären |
| A-K-21 | KL S. 5, Suwari Yokomen-uchi | KL zusätzlich 3. Kyu Nikyo, 2. Kyu Sankyo, Yonkyo | ? | fehlen in PO | klären |
| A-K-22 | PO S. 4, Hanmi-handachi Gyaku-hanmi 1. Kyu; KL S. 5 | PO: Kote-gaeshi (nukite / hineri); KL: nukite / kaeshi | ? | Variante abweichend | klären |
| A-K-23 | PO S. 4, Jo-dori Tsuki 1. Kyu; KL S. 6 | PO: „Kokyu-nage, mae (ura = nukido)“; KL: „mae (ura) 前（裏）“ | Kokyu-nage (mae, ura) | „nukido“ unklar, fehlt in KL | klären |
| A-K-24 | KL S. 5, Tanto-dori, Zeile „Yokomen-uchi 横面打“ | Zeile ohne Grade und Techniken | streichen | in PO kein Yokomen-uchi bei Tanto-dori | sicher |
| A-K-25 | KL S. 6, Jo-dori | ohne Kanji | 杖取り | Kanji fehlt in der Liste; im Heft ohne Kanji, bis bestätigt | klären |
| A-K-26 | KL S. 1, Katate-katate-dori (Ai-hanmi) und (Gyaku-hanmi) | 片手片手とり (Hiragana) | 片手片手取り | sonst überall 取り | Schreibweise |

## Aikido, Dan-Grade

| ID | Dokument und Stelle | Bisher | Vorschlag | Begründung | Status |
|---|---|---|---|---|---|
| A-D-01 | DL S. 1, Ai-hanmi | 愛半身 | 合半身 | KL schreibt 合半身; 愛 („Liebe“) ist vermutlich Umwandlungsfehler | Schreibweise |
| A-D-02 | PO S. 5–6 und DL | „irmi“ (PO S. 5 und DL S. 2, Yokomen 1. Dan), „Irmi-nage“ (PO S. 6 und DL S. 2, Suwari Ryote-dori 3. Dan), „Kokyu-nago“ (PO S. 6 und DL S. 3, Futari-gake 3. Dan), „Gyuaku-Hanmi“ (nur DL S. 3, Hanmi-handachi), „Uchi-'Te“ (nur PO S. 6, Sonstige) | irimi, Irimi-nage, Kokyu-nage, Gyaku-hanmi, Uchi-te | Tippfehler; im Heft korrigiert | sicher |
| A-D-03 | DL S. 1, Kata 4./5. Dan | Kogarashi 木枯らし, Shunpu 春風, beide mit Vermerk „unvollendet 未完“; PO S. 5 hat Kata nur bis 3. Dan (Tenchi) | ? | gehört Kogarashi in den 4. Dan, oder bleibt es draußen, solange die Form unvollendet ist? | klären |
| A-D-04 | DL S. 1, Ai-hanmi 3. Dan | zusätzlich Hiji-otoshi | ? | nur in DL | klären |
| A-D-05 | DL S. 1, Katate-muna-dori 4. Dan | Maki-arashi (巻嵐) | ? | nur in DL; PO S. 5 hat bei Katate-muna-dori keinen 4. Dan | klären |
| A-D-06 | DL S. 2, Yokomen-uchi 2. Dan | zusätzlich Kata-otoshi | ? | nur in DL | klären |
| A-D-07 | PO S. 5, Ushiro Ryote-dori 3./4. Dan; DL S. 2 | PO: Koshi-nage (Ikkyo-ura) / (Sankyo-ura wirken lassen); DL: Koshi-nage „Standard 標準技“ / ohne Zusatz | ? | Zusätze weichen ab | klären |
| A-D-08 | PO S. 5, Ryo-kata-dori 4. Dan; DL S. 2 | PO: „Koshi-nage (wie oben)“; DL: „erst Sankyo wirken lassen 先ず三教を効かせる“ | Koshi-nage (erst Sankyo wirken lassen), so im Heft | „wie oben“ verweist vermutlich auf Ryote-dori 4. Dan (Sankyo-ura wirken lassen); DL sagt dasselbe ausdrücklich | sicher |
| A-D-09 | DL S. 2, Suwari Katate-katate-dori gerade 4. Dan | Kime-modoshi nur in DL | ? | PO S. 6 hat Kime-modoshi nur bei Suwari Ryote-dori 4. Dan | klären |
| A-D-10 | PO S. 6 und DL S. 3, Hanmi-handachi 4. Dan | Kime-mawashi ohne Kanji | Kanji ergänzen | DL lässt die Spalte leer | klären |
| A-D-11 | DL S. 3, Sonstige Techniken | Zeilen Jiyu-waza (alle Tori-te / Uchi-te / Jiyu-seme), San'nin-gake und Theorie fehlen | ? | nur in PO S. 6 | klären |
| A-D-12 | DL S. 2, Tsuki 1. Dan | Zeile „1 Dan Kokyu-nage uchi-tenkan“ steht vor dem Angriff „Tsuki“, im Block Yokomen-uchi | zu Tsuki 1. Dan | Satzfehler; PO S. 5 hat sie bei Tsuki | sicher |
| A-D-13 | DL S. 3, Tanto-dori Yokomen-uchi und Suwari-waza (Ushiro-seme) Ryote-dori | Zeilen ohne Inhalt | streichen | in PO S. 6 nicht vorhanden | sicher |

## Aikido, beide Listen

| ID | Dokument und Stelle | Bisher | Vorschlag | Begründung | Status |
|---|---|---|---|---|---|
| A-X-01 | KL und DL | 切り落とし (KL S. 1)/切り落し, 隅落とし (KL S. 1)/隅落し, 入身投げ/入り身投げ (KL S. 4 Juji-irimi-nage, DL S. 2 Ryo-kata-dori), 袈裟懸け (KL S. 2)/袈裟掛け (DL S. 1), 巻き嵐 (DL S. 1 Gyaku-hanmi)/巻嵐 (DL S. 1 Katate-muna-dori) | im Heft je eine Form (`daten/begriffe.lua`) | eine Schreibweise je Begriff | Schreibweise |
| A-X-02 | PO, KL, DL | Groß-/Kleinschreibung und Bindestriche wechseln (Kamae-Undo, Jiyu-Waza, Katate-Katate-dori …) | einheitlich: erstes Wort groß, Rest klein, Teile mit Bindestrich | Lesbarkeit | Schreibweise |
| A-X-03 | DL S. 1–2, Zusätze „kote-hineri“ (Ai-hanmi 2. Dan, Gyaku-hanmi 3. Dan, Yokomen 3. Dan, Tsuki 2. Dan) | 篭手（小手） | 小手 | Doppelangabe | Schreibweise |

## Daitoryu, Kyu-Grade

| ID | Dokument und Stelle | Bisher | Vorschlag | Begründung | Status |
|---|---|---|---|---|---|
| D-K-01 | DK S. 2, Ushiro-seme Sotote-dori 5. Kyu | „Sankajo-Ushiro-gime“ | Sankajô-ushiro-gime | einziges „Sankajo“ ohne Zirkumflex | Schreibweise |
| D-K-02 | V „Daitoryu 5. Kyu“ | Zatori Yokomen-uchi Yoko-nage hat kein eigenes Kapitel | Kapitelmarke ergänzen | betrifft nur die App | klären |
| D-K-03 | DK S. 1–4 | „Uchi-te-dori“, „Soto-te-dori“ (Mae-seme), „Sotote-dori“ (Ushiro, Hanza); „Ryote-dori“, „Ryote-Katate-dori“ ohne ô (S. 2, Ushiro) | Uchite-dori/Sotote-dori wie in den Videos; Ryôte durchgehend | Einheitlichkeit | Schreibweise |
| D-K-04 | DK S. 1–3 | Aikiage-Aikisage, Makikomi-yokonage (S. 1), Jûji-Kiriotoshi (S. 1), Kotegaeshi, Kajimawashi, Fukiage, Munadori-Shômen-uchi (S. 3, Zatori) | Aiki-age-aiki-sage, Makikomi-yoko-nage, Jûji-kiri-otoshi, Kote-gaeshi, Kaji-mawashi, Fuki-age, Muna-dori-shômen-uchi | Einheitlichkeit | Schreibweise |
| D-K-05 | DK S. 4, Sonstige Techniken | Zeilen Futari-gake, Tachi-dori Shômen-uchi, Jô-dori Tsuki ohne Inhalt; ebenso Hanza-handachi (Ushiro) Ryô-kata-dori | im Heft weggelassen | keine Inhalte | sicher |
| D-K-06 | DK S. 2–4 | Einträge ohne Spiegelstrich bzw. ohne Leerzeichen: „Jûji-garami-nage (Jûji-uke)“ (S. 2), „Jûji-garami (jûji-uke)“ (S. 3), „-Aiki-nage“ (S. 2), „-Hiji-otoshi“ (S. 4) | im Heft ohne Striche | Satzfrage | Schreibweise |
| D-K-07 | DK S. 2, Tachiai Uchite | Zeile „Muna-dori-yokomen-uchi“ ohne Inhalt | weggelassen | im Zatori gibt es sie mit Jûji-garami | sicher |
| D-K-08 | DK S. 2, Tachiai Ushiro-seme | Zeilen „Katate-Muna-dori“ und „Kubi-jime“ ohne Inhalt | weggelassen | keine Inhalte | sicher |

## Daitoryu, Dan-Grade

| ID | Dokument und Stelle | Bisher | Vorschlag | Begründung | Status |
|---|---|---|---|---|---|
| D-D-01 | DD S. 4, Kaeshi-waza 3. Dan | „Tsuki – Shihôgage – Shihô-nage“ | Shihô-nage oder Shihô-gaeshi? | „Shihôgage“ gibt es nicht | klären |
| D-D-02 | DD S. 2–3 | ohne Spiegelstrich: Hiji-gatame (Ushiro Sotote-dori 1. Dan), Gassho-nage (Zatori Ryôte-dori 3. Dan), Hiza-giri, Hiji-otoshi-Yoko-nage (Okugi), Aiki-nage (Hanza Katate-kata-dori 3. Dan), Ryô-Nikajô-gime (Ryo-Sotote-dori 1. Dan) | ohne Striche | wie D-K-06 | Schreibweise |
| D-D-03 | DD S. 1–3 | Hiji-gime-Ipponzeoi (S. 1–2) vs. Hiji-gime-Ippon-zeoi (S. 2, Zatori); Muna-juji-nage (S. 1, Ryôte-dori 1. Dan) vs. Mune-juji-nage (S. 1, Ryôte-ryôkata-dori 1. Dan), beide ohne û; Oguruma, Tekubi-gime-Oguruma vs. O-guruma; Gassho-nage (DK: Gasshô-nage); „jodan-tsuki“ (S. 2, Tsuki 1. Dan) vs. „Jôdan-zuki“ (DK S. 2); Ryo-Yokomen-uchi, Ryo-Sotote-dori, Hizadate-Shomen-uchi ohne Zirkumflex | Ippon-zeoi, Jûji, O-guruma, Gasshô, Jôdan-zuki, Ryô-, Shômen; Muna/Mune klären | Muna (胸) und Mune (胸) stehen auf derselben Seite | klären |
| D-D-04 | DD S. 1–4 | Zeilen ohne Inhalt: Mune-ude-dori (S. 1), Ushiro Katate-muna-dori und Kubi-jime (S. 2), Zatori Katate-kata-dori, Hanza Ushiro Sotote-maeeri-dori (S. 3), Tantô-dori Uchioroshi/Yokomen-uchi/Tsuki (S. 4) | weggelassen | keine Inhalte | sicher |
| D-D-05 | DD S. 4, Kaeshi-waza | Kotegaeshi, Ikkajô-kimeosae, Shihônage, Hiji-gime-seoi-nage | Kote-gaeshi, Ikkajô-kime-osae, Shihô-nage | Einheitlichkeit | Schreibweise |
| D-D-06 | DD S. 2, Ushiro Ryôte-dori Okugi | „Koshi-nage uchi+soto“ | Koshi-nage (uchi + soto) | Satzform | Schreibweise |
| D-D-07 | DD S. 3, Futari-gake Ryô-uchite-dori 3. Dan | „Ryô-Yonkajô-gime(-nage)“ | ? | „gime“ oder „nage“, oder beides? | klären |

## Daitoryu, Japanisch

| ID | Dokument und Stelle | Bisher | Vorschlag | Begründung | Status |
|---|---|---|---|---|---|
| D-X-01 | DK, DD | keine Kanji | Vorschläge aus `daten/daitoryu-begriffe.lua`, im Heft hellgrau | müssen von einem Lehrer bestätigt werden; ohne Vorschlag: Karimata, Hô-daoshi, Seori (Liste: `build/fehlende-kanji.txt`) | klären |

## Videos (Kapitel ↔ Techniken)

Aus `tools/kapitel.py` (Titel im Video per Texterkennung, `daten/videos.yaml`), am 01.11.2026 an den
Bildern gegengelesen. Stimmt das Video mit den Daten nicht überein, steht hier, was das Video zeigt.
Die App spielt bis zur Klärung das zugeordnete Kapitel (Eintrag mit `pruefen: true`).

| ID | Video, Eintrag in den Daten | Das Video zeigt | Vorschlag | Begründung | Status |
|---|---|---|---|---|---|
| V-01 | Aikido 1.–3. Dan, Jiyu-waza (Alle Tori-te, Alle Uchi-te, Jiyu-seme, San'nin-gake) | Kapitel ohne Titel; nach 8 s „Futari-gake (zwei Angreifer) · Tsubasa-gaeshi“ bzw. „· Shiho-nage“ | Stelle von Hand setzen | Jiyu-waza hat im Video keinen Titel | klären |
| V-02 | Aikido 1. Kyu Ryote-katate-dori · Jiyu-waza; 2. Kyu Katate-katate-dori · Jiyu-waza (2×), Katate-muna-dori · Fuki-age | kein passendes Kapitel gefunden | im Video suchen | fehlt oder ohne Titel | klären |
| V-03 | Daitoryu 1. Kyu Ryôkata-dori · Soto-gake; Ryôte-kubi-jime · Niô-daoshi | „Shômen-uchi · Karami-nage“ (Titel erst nach 8 s); „Ryôte-kubi-jime · Aiki-nage“ | Daten oder Zuordnung prüfen | Name im Video anders | klären |
| V-04 | Daitoryu 2. Kyu Tsuki/Ryôte-dori · Haimen-ude-gatame-otoshi, Tekô-osae · Nikajô-kime-osae, Tsuki · Kote-gaeshi | Kapitel mit spätem Titel: „Sotote-dori · Hiji-awase-otoshi“, „Uchite-dori · Irimi-nage“, „Sotote-dori · Shihô-nage“, „Eri-dori · Hiji-otoshi“ | Reihenfolge im Video mit DK vergleichen | Video folgt hier nicht der Liste | klären |
| V-05 | Daitoryu 2. Kyu Tantô-dori · Nikajô-kime-osae, Sankajô-kime-osae | „Uchioroshi · Sankajo-Ushiro-gime“, „Yokomen-uchi · Sankajo-Ushiro-gime“ | Daten prüfen | Angriff und Technik anders benannt | klären |
| V-06 | Daitoryu 3. Kyu Sotote-dori · Ude-gatame-otoshi; Tantô-dori · Kote-gaeshi | „Sotote-dori · Sankajô-ushiro-kime-nage“; „Tsuki · Kote-otoshi“ | Daten prüfen | Name im Video anders | klären |
| V-07 | Daitoryu 4. Kyu Yokomen-uchi · Fuki-age | „Yokomen-uchi · Hô-daoshi“ | Daten prüfen | Name im Video anders | klären |
| V-08 | Daitoryu 1. Dan Ryôte-muna-dori · Koshi-guruma; Ryôte-ryôkata-dori · Mune-jûji-nage; Ryôte-dori · Ude-garami; Ryô-morote-dori · Aiki-otoshi | „Ryôte-Muna-dori / Ryô-eri-jime · Koshi-nage“; „Ryôkata-dori · Jûji-garami-nage“; „Ryôte-dori · Karami-nage“; „Ryô-Morote-dori · Aiki-nage“ | Daten prüfen | Namen im Video anders | klären |
| V-09 | Daitoryu 2. Dan Tsuki · Yoko-otoshi; Jô-dori · Hikikomi-nodo-zuki | „Tsuki (Jôdan) · Hiji-otoshi“; Angriff „Tsuki“ | Daten prüfen | Name bzw. Angriff im Video anders | klären |
| V-10 | Daitoryu 3. Dan Jô-dori · Kata-gime-nage, Ura-irimi-nodo-zuki | Angriff „Tsuki“, „Kata-gime“, „Ura-irimi-tenkan-Nodo-zuki“ | Daten prüfen | Angriff im Video anders | klären |
| V-11 | Daitoryu Okugi Tsuki · Hiji-gime-nage; Ryôte-dori · Kakae-gime-nage; Ryô-morote-dori · Aiki-kôhô-otoshi | „Sotote-dori · Hiji-gime-nage“ (spät); „Ryôte-dori · Juji-Tekubi-gime-modoshi“; „Ryô-Morote-dori · Nikajô-kime-osae“ | Daten prüfen | Namen im Video anders | klären |
| V-12 | Daitoryu ohne Kapitel: 1. Kyu Muna-dori-shômen-uchi · Jûji-garami; 3. Kyu Sotote-dori · Yonkajô-kime-osae, Ryôte-muna-dori · Kataude-shibori-kime-nage; 5. Kyu Yokomen-uchi · Yoko-nage; 1. Dan Tsuki · Kata-otoshi, Ryô-yokomen-uchi · Aiki-nage; 2. Dan Ryôte-dori · Tai-azuke; 3. Dan Tsuki · Ude-gatame | kein passendes Kapitel | im Video suchen | fehlt im Video oder anders benannt | klären |
| V-13 | Shinki Toho, 26 Kamae | die Kamae-Übung am Stück (Video Suburi, Kapitel 5–7) | Stellen je Kamae von Hand setzen | ein Kapitel für alle | klären |
| V-14 | Shinki Toho, Kiriotoshi | in keinem der drei Videos | ohne Video lassen | kein Kapitel | sicher |

## Shinki Toho

| ID | Dokument und Stelle | Bisher | Vorschlag | Begründung | Status |
|---|---|---|---|---|---|
| S-01 | OK S. 9, Überschrift Form 5 | Überschrift: „(5. Hidari-gote)“; Fließtext OK S. 10: „schlägt das Migi-gote“; OT und OK-Übersicht: Migi-gote | Migi-gote, so im Heft | nur die Überschrift weicht ab | sicher |
| S-02 | V Shoden, Titelkarte Form 5 | 付き込み正眼破り (geprüft) | 突き込み? | 突き („stoßen“) passt zu Tsuki-komi; 付き („anhaften“) steht aber so auf der Karte | klären |
| S-03 | OK S. 2, Übersicht Form 9, Spalte Zanshin | Übersicht: Gedan; OT und Fließtext OK S. 11: „Zanshin In“ | In, so im Heft | nur die Übersicht weicht ab | sicher |
| S-04 | OK S. 3, Übersicht Form 16, Zanshin | Übersicht: Gedan; OT und Fließtext OK S. 14: „Zanshin Sasage-Dachi“ | Sasage-dachi, so im Heft | nur die Übersicht weicht ab | sicher |
| S-05 | OK S. 3, Übersicht Form 21, Zanshin | Übersicht: Migi Kiriharai; OT und Fließtext OK S. 16: „Zanshin Migi Harai-dachi“ | Migi Harai-dachi, so im Heft | nur die Übersicht weicht ab | sicher |
| S-06 | OK S. 3, Übersicht Form 23, Treffer | Übersicht: Hara; OT und Fließtext OK S. 17 („32. Mizo'ochi“) | Mizo'ochi, so im Heft | nur die Übersicht weicht ab | sicher |
| S-07 | OK S. 3, Übersicht Form 22, Treffer | „Migi shita“; OT: „Migi-gote shita (von unten)“ | Migi-gote von unten, so im Heft | Kurzform, kein Widerspruch | Schreibweise |
| S-08 | OT, OK, V | Into (OT, OK S. 10)/Intô (OK S. 2)/Intou (V); Jûichi; Kasumi fûji (OT, OK)/Kasumi-fuji (V); Honshô/Honshou (V); Hitotsu-gachi/Hitotsu-kachi (OK S. 3 und S. 11); Futatsu-gachi (Futatsu-kachi) (OK S. 10); Chu-Seigan (OT)/Chû-Seigan (OK) | Intô, Jûichi, Kasumi-fûji, Honshô, Hitotsu-gachi, Futatsu-gachi, Chû-seigan | Einheitlichkeit | Schreibweise |
| S-09 | Kamae-Tabelle | Seigan kommt zweimal vor | so lassen | ist ein Ablauf; im Heft erklärt | sicher |
| S-10 | V, OT | Kanji der Formen 22–43 fehlen | Quelle suchen (Titelkarten Okuden/Jun-Menkyo-Videos) | keine Vorlage vorhanden | klären |
| S-13 | OK S. 3, Übersicht Form 24, Treffer | Übersicht: Migi; OT und Fließtext OK S. 17: „schlägt S dessen Hidari-gote“ | Hidari-gote, so im Heft | nur die Übersicht weicht ab | sicher |
| S-14 | OK S. 3, Übersicht Form 18, Zanshin | Übersicht: Yô; OT und Fließtext OK S. 15: „Zanshin Gedan“ | Gedan, so im Heft | nur die Übersicht weicht ab | sicher |
| S-15 | V Chuden, Titelkarte Form 16 | 十六番 上段霞み | so übernommen | im ersten Datenstand fehlte み (Abschreibfehler von mir), jetzt korrigiert | erledigt |
| S-12 | Grade und Prüfungsberechtigung | Namen der Ko-dachi-, Tantô- und Mutô-Formen unbekannt | ergänzen | für Hon-Menkyo/Menkyo-Kaiden nötig | klären |
