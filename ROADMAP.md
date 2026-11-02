# Fahrplan

Sechs Schritte bis Mitori 1.0 (aus `entwuerfe/app-konzept.html`, Stand 01.11.2026). Wie jede Ansicht
aussieht und sich verhält, steht im Clickdummy (`entwuerfe/clickdummy/index.html`) und in
`entwuerfe/app-entscheidungen.md`; beides ist die Vorlage, nicht dieser Plan.

| Schritt | Inhalt | Fertig, wenn |
|---|---|---|
| 0 · Daten | Programmdaten als eine Quelle (`daten/*.yaml` → `tools/daten.py`), Kapitel aller Videos den Techniken zuordnen (ffprobe + tesseract, gegenlesen) | jede Technik hat ihre Videostelle |
| 1 · Abspielen | Repo-Gerüst aus Plasmai ✓, Startmenü, Programm, Player mit Zeitlupe und Schleife; Videos vorerst aus einem lokalen Ordner | APK aus der CI spielt jede Technik ab |
| 2 · Üben | Einschätzen (vier Punkte), Notizen mit Fotos und Sprachnotizen, Heute üben, Training als Playliste, Statistik, Suche | eine Woche im Dojo benutzt |
| 3 · Prüfen und Lernen | Prüfungsmodus (Tafel), Technik-Landkarte, Glossar mit Lexikon und Aussprache, Kommentare Shinki Toho | Probeprüfung damit gemacht |
| 4 · Bundles und Termine | `tools/bundle.py` (Beschnitt, Verschlüsselung, Manifest), Code-Eingabe und QR, Download; Lehrgänge aus ICS (je Lehrgang: in den Gerätekalender eintragen, Ausschreibung öffnen), Meine Grade, Wartezeiten, Lehrgangsnotizbuch, Sicherung (Export, Auto Backup, WebDAV-Abgleich) | ein Bundle per Code auf einem fremden Telefon geladen |
| 5 · Release | Demo-Modus mit Studio-Weber-Daten, Update-Check, Screenshots, Landing Page `docs/`, GitHub-Release | v1.0.0 auf GitHub und der Landing Page |

## Stand

- Schritt 0: Daten aller drei Sparten in `daten/`, geprüft von `tools/daten.py`; Heft baut (`heft/build.sh`).
  Videostellen aller Videos per `tools/kapitel.py` in `daten/videos.yaml`; offen: die Einträge mit
  `pruefen: true` gegenlesen, Abweichungen Video ↔ Daten in `KORREKTUREN.md` (V-…) klären.
- Schritt 1: Startmenü (Kachel Programm), Programm (Sparte, Grad, Liste), Technik-Blatt, Kommentar,
  Player (Schleife, vier Tempi, Einzelbild, Doppeltipp, Querformat), Videos aus einem lokalen Ordner.
  Offen: Gesten für Helligkeit und Lautstärke (Plattform-Schicht), Test auf dem Telefon. Tablet quer: Startmenü als
  Raster mit vier Spalten; Programm und Player klein auf einer Seite (Liste links, Player rechts mit Kommentar). Offen
  dort: rechts das Blatt bei Techniken ohne Video, Player groß mit Titel und Steuerung über/unter dem Video,
  Kommentar als Lesespalte. Tastatur im Programm: Tab erreicht nur die angelegten Zeilen der virtuellen Liste und
  verliert den Fokus beim Scrollen (Auswahl mit Auf/Ab statt Scrollen, oder Zeile in den Blick holen).
- Schritt 2: Einschätzen 1–4 im Technik-Blatt und im Player (Übungsprotokoll in SQLite, jede Einschätzung ein
  Eintrag mit Zeit und Gerät), Stand als vier Punkte in der Programmliste, Stand je Grad als Leiste mit Legende.
  Heute üben (Wiederholen nach Plan, Abstände je Stufe einstellbar, Kachel im Startmenü) und Training (Playliste der
  fälligen Techniken, Einschätzen schaltet weiter). Notizen je Technik (Text; nach Tagen, lang drücken zum
  Bearbeiten/Löschen, letzte Notiz im Blatt, Eingabe im Player), „Alle Notizen“ (nach Tagen, Filter je Sparte; im
  Programm und in den Notizen einer Technik). Suche (Techniken aller Sparten und Grade, eigene Notizen; Groß/klein,
  Akzente, ß, Bindestriche und Okurigana egal; auch Angriffszusatz und Gruppe) über die Lupe im Startmenü. Statistik
  (Kachel im Startmenü): letzte 30 Tage (Übungstage, Techniken geübt), Übungstage als Kalender (halbes Jahr,
  Legende, Tooltip je Tag), Stand je Grad einer Sparte. Offen: Fotos und Sprachnotizen an Notizen
  (Plattform-Schicht), Lehrgangsnotizen in „Alle Notizen“ (mit Schritt 4); in der Statistik „Meine Grade“
  (mit Schritt 4), „alle Jahre“.
- Schritt 3: Prüfungsmodus (Kachel 試 im Startmenü, „Probe <Grad>“): Sparte, Grad (nur mit Prüfungsstoff, je
  Anzahl Punkte), Anzahl (alle / 8 / 12 / 15), zufällige Reihenfolge, 2–5 Stichproben aus früheren Graden am
  Anfang (markiert); Einstellungen bleiben. Shinki Toho: Shoden prüft Kamae als einen Punkt (setzt den Stand
  aller 26 Kamae) und Ô-dachi 1–11, höhere Grade ihre Formen. Runde als Tafel (Gruppe, Angriff, Technik; Shinki
  Toho nur Technik), Weiter mit Leertaste/Enter, Tablet größer. Am Ende jede Technik 1–4 einschätzen; Übernehmen
  setzt den Stand (Übungsprotokoll) und speichert die Runde (SQLite, Schema 3), die Statistik zeigt den Schnitt
  der letzten 8 Runden je Sparte. Offen: Hinweis auf eine vorgemerkte Prüfung (mit Schritt 4, Lehrgänge).
- Schritt 3: Glossar (Kachel 辞 im Startmenü, auch die Lupe): ohne Eingabe A–Z mit Kanji und Übersetzung, mit
  Eingabe vor Techniken und Notizen (auch deutsche Wörter; gleiche Normalisierung wie die Suche). Daten: Aiki-Budo-
  Lexikon plus die Begriffe aus `daten/begriffe.yaml`, die es nicht hat (Schreibweise in Klammern und oo wie o
  zählen als dasselbe Wort; Begriffe mit offener Kanji-Frage wie A-K-04 und die ungeprüften Daitoryu-Bausteine
  bleiben draußen), je Wort eine Lesung in Hiragana aus der Umschrift (`tools/daten.py`). Aussprache per Qt
  TextToSpeech mit japanischer Stimme (Kanji, sonst die Lesung); ohne Stimme oder Sprachausgabe kein Knopf, ein
  Hinweis. Sprechen kann: Android (Plugin im APK, japanische Stimme der Sprachausgabe nötig), Linux mit qt6-speech
  und einer japanischen Stimme (speech-dispatcher). Nicht: AppImage (ohne qtspeech gebaut), Flatpak (die
  KDE-Runtime hat nur das Test-Plugin). Offen: Aussprache auf dem Telefon prüfen, qtspeech für AppImage und
  Flatpak, A–Z-Sprungleiste, Lesungen ohne lange Vokale (Umschrift ohne ô, „aikido“ → あいきど), in der Suche
<<<<<<< HEAD
  Kommentare und Lehrgänge. Noch nicht begonnen: Landkarte.
=======
  Kommentare und Lehrgänge. Noch nicht begonnen: Prüfungsmodus.
- Schritt 3: Technik-Landkarte (Kachel 図 im Startmenü): Sparte, „bis“ Grad (anfangs der des Programms), je Gruppe
  ein Raster Technik × Angriff über alle Grade bis dahin (Gruppen gleichen Titels aus Kyu- und Dan-Graden zusammen),
  jeder Platz in der Farbe seiner besten Stufe, leer, wo die Kombination nicht vorkommt; Shinki Toho als nummerierte
  Ô-dachi-Formen. Tippen öffnet das Technik-Blatt, Überfahren zeigt Technik, Angriff, Grad und Stand; Tab erreicht
  einen Platz je Raster, die Pfeiltasten wandern weiter. Breite Raster scrollen waagerecht, die Technik-Namen bleiben
  stehen. Raster: `KanteHeatGrid` (Kante 1.63, dafür ergänzt).
>>>>>>> origin/main

## Lehrgänge (Schritt 4)

- **In den Kalender:** jeder Lehrgang aus dem Kalender lässt sich in den Kalender des Geräts eintragen (Titel, Zeit, Ort,
  Beschreibung; Plattform-Schicht: Android über einen Kalender-Intent, Plasma als ICS-Datei an die
  Kalender-App).
- **Ausschreibung:** steht in der Beschreibung des ICS-Eintrags eine Zeile „Ausschreibung: URL“, zeigt der
  Kopf des Notizbuchs (wo auch „In den Kalender“ sitzt) einen Knopf, der die offizielle
  Lehrgangsausschreibung öffnet; ohne diese Zeile kein Knopf. Stand 02.11.2026 tragen 4 von 24 Terminen die Zeile.
  Beim Lesen: Zeilen erst entfalten (CRLF + Leerzeichen), Escapes (`\n`, `\,`) auflösen, die URL endet am
  Zeilenende und wird nicht umkodiert (sie enthält `&` und `%2F`). Gesucht wird im Text der `DESCRIPTION`, nicht
  im HTML des `ALTREP` (dort `&amp;`), und nur die des Termins: eine Erinnerung (`VALARM`) hat eine eigene
  `DESCRIPTION` („Mozilla Standardbeschreibung“). Übernommen werden nur `http(s)`-Links.

## Kante-Bedarf

Fehlt in Kante und kommt dort hinzu (GUI-Regel), bevor Mitori es endgültig nutzt:
- Seiten-Scope: Kirigamis ScrollablePage erbt die Kante-Farben nicht (`Theme.inherit: false`); jetzt je Seite
  `KanteScope` plus Seitengrund. Besser eine Option in `KanteScope` (auch für System/Kante Light: View- statt
  Window-Farbe bei Listen) und die globale Kopfleiste des pageStack in Kante-Farben (heute Plattformfarbe).

## Querschnitt

- **Theme Shinki** (Farben der Shinki News) als Rollen in Kante, wählbar neben Kante.
- **Videos ausliefern**: beschnitten (Aikido `crop=720:372:0:64`, Daitoryu `crop=720:374:0:50`,
  Shinki Toho ohne), HEVC, als versionierte Bundles im generischen Gitea-Paket.
- **Übersetzungen** deutsch und englisch von Anfang an.
- **Später**: Stores (F-Droid, Flathub), weitere Statistik-Auswertungen.
