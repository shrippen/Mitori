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
  Wischen hoch/runter links: Helligkeit, rechts: Lautstärke (`platform/devicelevels`: Android Fenster-Helligkeit
  und AudioManager, Linux Helligkeit über Solid, Ton des Players). Offen: Test auf dem Telefon. Tablet quer: Startmenü als
  Raster mit vier Spalten; Programm und Player klein auf einer Seite (Liste links, Player rechts mit Kommentar). Offen
  dort: rechts das Blatt bei Techniken ohne Video, Player groß mit Titel und Steuerung über/unter dem Video,
  Kommentar als Lesespalte. Tastatur im Programm: Tab geht Zeile für Zeile durch die ganze Liste und hält die
  Zeile im Blick, Ende springt beim ersten Druck ans Ende.
- Schritt 2: Einschätzen 1–4 im Technik-Blatt und im Player (Übungsprotokoll in SQLite, jede Einschätzung ein
  Eintrag mit Zeit und Gerät), Stand als vier Punkte in der Programmliste, Stand je Grad als Leiste mit Legende.
  Heute üben (Wiederholen nach Plan, Abstände je Stufe einstellbar, Kachel im Startmenü) und Training (Playliste der
  fälligen Techniken, Einschätzen schaltet weiter). Notizen je Technik (Text; nach Tagen, lang drücken zum
  Bearbeiten/Löschen, letzte Notiz im Blatt, Eingabe im Player), „Alle Notizen“ (nach Tagen, Filter je Sparte und Lehrgänge, Zeilen in C++; im
  Programm und in den Notizen einer Technik). Suche (Techniken aller Sparten und Grade, eigene Notizen; Groß/klein,
  Akzente, ß, Bindestriche und Okurigana egal; auch Angriffszusatz und Gruppe) über die Lupe im Startmenü. Statistik
  (Kachel im Startmenü): letzte 30 Tage (Übungstage, Techniken geübt), Übungstage als Kalender (halbes Jahr,
  Legende, Tooltip je Tag), Stand je Grad einer Sparte. Fotos und Sprachnotizen an Notizen (mit Schritt 4).
  Offen: in der Statistik „Meine Grade“
  (mit Schritt 4), „alle Jahre“.
- Schritt 3: Prüfungsmodus (Kachel 試 im Startmenü, „Probe <Grad>“): Sparte, Grad (nur mit Prüfungsstoff, je
  Anzahl Punkte), Anzahl (alle / 8 / 12 / 15), zufällige Reihenfolge, 2–5 Stichproben aus früheren Graden am
  Anfang (markiert); Einstellungen bleiben. Shinki Toho: Shoden prüft die Kamae-Übung (ein Eintrag) und
  Ô-dachi 1–11, höhere Grade ihre Formen. Runde als Tafel (Gruppe, Angriff, Technik; Shinki
  Toho nur Technik), Weiter mit Leertaste/Enter, Tablet größer. Am Ende jede Technik 1–4 einschätzen; Übernehmen
  setzt den Stand (Übungsprotokoll) und speichert die Runde (SQLite, Schema 4), die Statistik zeigt den Schnitt
  der letzten 8 Runden je Sparte; eine vorgemerkte Prüfung steht oben, ihr Grad ist einen Tipp entfernt.
- Schritt 3: Glossar (Kachel 辞 im Startmenü, auch die Lupe): ohne Eingabe A–Z mit Kanji und Übersetzung, mit
  Eingabe vor Techniken und Notizen (auch deutsche Wörter; gleiche Normalisierung wie die Suche). Daten: Aiki-Budo-
  Lexikon plus die Begriffe aus `daten/begriffe.yaml`, die es nicht hat (Schreibweise in Klammern und oo wie o
  zählen als dasselbe Wort; Begriffe mit offener Kanji-Frage wie A-K-04 und die ungeprüften Daitoryu-Bausteine
  bleiben draußen), je Wort eine Lesung in Hiragana aus der Umschrift (`tools/daten.py`). Aussprache per Qt
  TextToSpeech mit japanischer Stimme (Kanji, sonst die Lesung); ohne Stimme oder Sprachausgabe kein Knopf, ein
  Hinweis. Sprechen kann: Android (Plugin im APK, japanische Stimme der Sprachausgabe nötig), Linux mit qt6-speech
  und einer japanischen Stimme (speech-dispatcher), das AppImage (bringt das speech-dispatcher-Plugin mit). Nicht:
  Flatpak (die KDE-Runtime hat nur das Test-Plugin). Offen: Aussprache auf dem Telefon prüfen, qtspeech für
  Flatpak, A–Z-Sprungleiste, Lesungen ohne lange Vokale (Umschrift ohne ô, „aikido“ → あいきど), in der Suche
  Kommentare und Lehrgänge.
- Schritt 3: Technik-Landkarte (Kachel 図 im Startmenü): Sparte, „bis“ Grad (anfangs der des Programms), je Gruppe
  ein Raster Technik × Angriff über alle Grade bis dahin (Gruppen gleichen Titels aus Kyu- und Dan-Graden zusammen),
  jeder Platz in der Farbe seiner besten Stufe, leer, wo die Kombination nicht vorkommt; Shinki Toho als nummerierte
  Ô-dachi-Formen. Tippen öffnet das Technik-Blatt, Überfahren zeigt Technik, Angriff, Grad und Stand; Tab erreicht
  einen Platz je Raster, die Pfeiltasten wandern weiter. Breite Raster scrollen waagerecht, die Technik-Namen bleiben
  stehen. Raster: `KanteHeatGrid` (Kante 1.63, dafür ergänzt; Formenraster füllt die Breite seit Kante 1.64).
- Schritt 4: Lehrgänge (Kachel 講 im Startmenü, nennt den nächsten Termin): die kommenden Termine aus dem öffentlichen
  Lehrgangskalender (ICS, alle sechs Stunden neu geladen, offline vom Gerät), ein Notizbuch je Termin (beim ersten
  Öffnen angelegt, mit einer Kopie des Termins) und eigene Lehrgänge ohne Kalendereintrag. Im Notizbuch: Tage, Ort,
  Lehrer (aus „mit …“ der Beschreibung), „In den Kalender“ (Plattform-Schicht `platform/devicecalendar`: Android
  über den Kalender-Intent, sonst als ICS-Datei an die Kalender-App), „Ausschreibung“ (aus der Zeile
  „Ausschreibung: URL“), Einträge nach Tagen (lang drücken: ändern, löschen), Notizbuch löschen. Gespeichert in
  `lehrgaenge.sqlite` (`store/coursebook`, eigenes Schema), jede Zeile mit Zeit und Gerät für den Abgleich.
  Die Einträge stehen auch in „Alle Notizen“ (Filter „Lehrgänge“). „Exportieren“: das Notizbuch als ZIP
  (`notizbuch.md` nach Tagen, `bilder/`, `audio/`; `store/zipwriter`, `store/notebookexport`) in eine gewählte Datei.
- Schritt 4: Fotos und Sprachnotizen an Notizen einer Technik und an Lehrgangseinträgen: Foto mit der Kamera
  (`CameraPage`) oder aus einer Datei, Sprachnotiz (AAC in MP4) direkt in der Eingabezeile; Anzeige in Notizen,
  Notizbuch und „Alle Notizen“, Sprachnotiz zum Abspielen. Dateien in `medien/` (`store/mediastore`, Fotos höchstens
  2048 px), mit der Notiz gelöscht. Übungsprotokoll Schema 5, Lehrgänge Schema 3 (Spalte `media`). Android fragt nach
  Kamera und Mikrofon beim ersten Mal. Offen: Test auf dem Telefon (Kamera, Mikrofon, Bild aus der Galerie).
- Schritt 4: Meine Grade (in Lehrgänge oben): je Sparte die abgelegten Grade mit Tag und Ort, der nächste Grad mit
  Wartezeit (`wartezeit` aus `abstand` der Programmdaten; Shinki Toho Shoden ab Trainingsbeginn, ohne Datum).
  Im Notizbuch eines kommenden Lehrgangs „Prüfung hier ablegen …“ (Sparte, Grad, Wartezeit gegen den ersten Tag);
  Lehrgänge zeigen die geplanten Prüfungen, die Kachel 講, Heute üben und der Prüfungsmodus die nächste.
  `lehrgaenge.sqlite` Schema 2 (Tabelle `grades`, Spalten `exam_*` an `courses`). Offen: „Meine Grade“ in der
  Statistik, Trainingsbeginn für Shinki Toho.
- Schritt 4: Sicherung als Datei (Einstellungen → Sicherung): `mitori-sicherung-JJJJ-MM-TT.zip` mit beiden Datenbanken
  (`VACUUM INTO`), `medien/` und `manifest.json` (`store/backuparchive`, `store/zipreader`, `services/backup`).
  Einlesen führt zusammen (`store/tablemerge`): Einschätzungen und Prüfungsrunden als Vereinigung, Notizen,
  Notizbücher, Einträge und Grade per uid, das neuere `changed` gewinnt, Löschmarken eingeschlossen; zwei Notizbücher
  eines Termins werden eins, ein Grad doppelt der neuere. Vorher zeigt ein Dialog, was neu und was geändert ist.
  DVD-Codes und Inhaltsschlüssel bleiben draußen. Android Auto Backup sichert die Datenbanken ohne Medien.
  Offen: WebDAV/Nextcloud-Abgleich mit Konflikten (Clickdummy „Telefon / Tablet / Beide behalten“).
- Schritt 4: Video-Bundles (Einstellungen → „Videos und DVD-Codes“, Hinweis im Programm „Code eingeben“): das öffentliche
  Manifest (Gitea-Paket `mitori-manifest`) listet die Bundles; der Code einer DVD (20 Zeichen, Prüfzeichen) schaltet
  eins frei, Mitori lädt, prüft (SHA-256, Blöcke mit Tag) und entschlüsselt seine Videos aufs Gerät
  (`store/bundlecrypto` mit Monocypher, `store/bundleshelf`, `services/bundles`). Nur im WLAN (Schalter, „Jetzt laden“
  einmal ohne), einzeln entfernen (Code bleibt), neue Version als Update mit Hinweis im Startmenü. Videos aus einem
  Bundle sind schon beschnitten. Werkzeug `tools/bundle.py` (build, codes, sperren, pruefen, hochladen), Bundles in
  `tools/bundles.yaml`, Geheimes in `../bundle-geheim`. Offen: Bundles bauen und hochladen (Owner: Zustimmung des
  Verbands), QR-Code scannen, Freischalten auf dem Telefon prüfen (Argon2id mit 64 MiB).

## Offene Punkte (Stand 02.11.2026)

Kleinigkeiten aus Reviews und Tests, je ein Folge-PR oder beim nächsten Bearbeiten der Datei.

- **Landkarte**: Varianten eines Platzes (z. B. Ikkyo und Ikkyo ura) sind nur über das erste Blatt erreichbar:
  Frage an den Owner, ob sie einzeln erreichbar sein sollen. Datenfragen A-X-04 und D-D-08 (`KORREKTUREN.md`):
  geklärt, legen sich Spalten bzw. Raster zusammen.
- **Player-Gesten** (Review #35, mit dem Test auf dem Telefon): Android-Helligkeit über 255 (manche Hersteller)
  auf 0..1 begrenzen; Linux: Solid-Aufrufe asynchron, `brightnessMax` einmal je Wisch; beim Verlassen nicht
  zurücksetzen, wenn die Helligkeit inzwischen mit den Tasten geändert wurde.

- **Video-Bundles** (Review #33): ein Update tauscht die Ordner über `.old` statt erst zu löschen; „wartet auf
  WLAN“ startet beim Wechsel ins WLAN von selbst (oder anderer Text); HTTP-Status vor dem Entschlüsseln prüfen
  (sonst „beschädigt“ statt „nicht gefunden“); `tools/bundle.py hochladen` als Stream.

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
