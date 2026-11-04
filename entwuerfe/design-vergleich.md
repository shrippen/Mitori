# Clickdummy und App: Designvergleich

Stand: 03.11.2026. Quellen: `entwuerfe/clickdummy/` (Vorlage, Theme Shinki), `app/qml/` (Ist-Stand),
`shinki/qml/Shinki/` (Shinki-Bausteine), `shinki/README.md` (Bible), Screenshots der App auf dem
Pixel 6 (Leinen und Shinki).

Der Clickdummy ist die Vorlage **für den Shinki-Look** (Papier, Tusche, ein Rot, Serif). Kante ist ein
**eigenes Theme** (dunkel, scharfe Kanten, gelber Akzent) mit eigener Bible und eigenen Tokens; es
darf und soll anders aussehen. Die beiden Stile werden deshalb getrennt geprüft:

- **Shinki** – Vorlage: der Clickdummy und `shinki/README.md` + `shinki/tokens.json`.
- **Kante** – Vorlage: die Kante-Bible (`../Kante`, `AGENT-RULE.md`) und die Kante-Tokens.
  Nicht in dieser Liste: hier geht es nur darum, dass die App **keine Kante-Werte in den
  Shinki-Look und keine Shinki-Werte in den Kante-Look** mischt (siehe Abschnitt 3).
- **Breeze/Material** – die Plattformlooks; sie nutzen die Controls des Systems.

Gemeinsam gilt für alle: eine Seite liest eine **Rolle** (`Look.…`), nie eine Farbe oder eine Zahl
aus einem der beiden Themes.

Legende: **CD** = Clickdummy, **App** = Ist-Stand, **Bibel** = `shinki/README.md`.
Die Spalte „App" nennt den Ist-Stand im **Shinki**-Look, weil der Clickdummy ihn zeigt.

---

## 1. Vergleich je Ansicht

| Ansicht | CD (Vorlage) | App (Ist) | Bewertung |
|---|---|---|---|
| Startmenü | Kopf mit Logo (神氣, rotes Quadrat), Kicker „Mitori 見取り", Zahnrad rechts, Hinweisboxen, 2-Spalten-Kacheln, große Programm-Kachel | Kacheln einspaltig mit Kanji-Glyphen, Suchen/Zahnrad in der unteren Leiste, Logo fehlt | teilweise |
| Programm | Segmentierung, Grade-Chips mit Stand, Fortschrittskarte „Prüfungsreif x/y" mit 5-Teil-Balken + Legende, Hinweis „Prüfungsvideos kaufen", Infokarte Shinki Toho, Gruppenkopf als roter Kicker, Angriffszeile, Zeilen mit Serif-Name + 4 Punkten + rotem Play-Kreis | Segmentierung ✓, Chips ✓, GradeStand ✓ (`ProgrammePage.qml:118`), Hinweis ✓ (`:125`), Technikname in **Sans**, Punkte als **4 leere Quadrate**, Play-Knopf ohne Rot-Bezug | teilweise |
| Technik-Blatt | Bottom-Sheet mit Griff, Kicker, Titel 25 px **Serif** + Kanji, Meta, 4er-Bewertung, letzte Notiz, Hinweis, Buttons „▶ Video", „Notizen (n)", „Kommentar" | `Kirigami.Dialog` (`TechniqueSheet.qml:10`), Desktop-Dialog statt Bottom-Sheet | Abweichung |
| Player (hoch) | Titel-Kicker + Serif-Titel, Zeitleiste mit rotem Griff und tabularen Zeiten, 5er-Transport mit 52-px-Pause, Tempo-Zeile (4 Tempi), 3 Toggles, Bewertungskarte, Notizzeile + roter Senden-Knopf | `PlayerView.qml`: Zeitleiste ✓, 4 Tempi ✓, Schleife ✓, Doppeltipp ✓, Wischen ✓, Transport über `LookTransportBar` | Optik |
| Querformat | Video fast vollflächig, 62-px-Schiene rechts (Hochformat, ◀︎/▶︎, Pause 40 px, Tempo, ↻), Zeitleiste unten über die Breite, Doppeltipp-Anzeige | `PlayerView.qml:168-172`: 2 Spalten (Bild + Steuerung rechts) – Struktur ✓, Schienen-Optik ✗ | teilweise |
| Kommentar | Serif-Titel + Kanji, h3-Kicker rot, Fließtext **16 px Serif**, Fußnoten mit 2-px-Linie links, Quellenzeile | `CommentPage.qml`: QQC2.Label in Standard-Schrift, keine Serif-Rolle | Abweichung |
| Notizen / Alle Notizen | Tagesgruppen, Serif-Notiztext, Foto-/Audio-Platzhalter, Tag-Chip, Composer | vorhanden (`NotesPage`, `AllNotesPage`), Look-Bausteine | ✓ |
| Heute üben / Prüfungsmodus / Runde / Prüfende | Karten, Board, Bewertung am Ende | vorhanden, Look-Bausteine | ✓ |
| Statistik / Meine Grade / Jahre | Heatmap, Kalender, KPI | vorhanden, `LookCalendarHeatmap` | ✓ |
| Lehrgänge / Notizbuch / Neuer Lehrgang | Listen, Formular | vorhanden, aber `Kirigami.FormLayout` (`CourseNewPage.qml:45`, `SettingsPage.qml:22`) | Formular-Optik |
| Landkarte | Raster Techniken × Angriffe | `TechniqueMapPage.qml`: `KanteScope` direkt, **keine Shinki-Variante** | Lücke |
| Glossar & Suche | A–Z, Kanji, Aussprache | vorhanden | ✓ |
| Einstellungen / Videos / Sicherung / Konflikt / Darstellung / Code | Seiten mit Formularen, Listen, Karten | vorhanden, `Kirigami.FormLayout`, `QQC2.Label` | Formular-Optik |
| Funktionumfang gesamt | 13 Ansichten, 5 davon fertig | **alle Ansichten der Vorlage sind in der App vorhanden** | ✓ |

**Fazit:** Die App hat den **Funktionsumfang** des Clickdummys vollständig. Die Unterschiede sind
fast alle im **Look**: Kopfzeile, Typografie, Listenzeichen, Bottom-Sheet, Formulare und die
Lücken im Shinki-Layer.

---

## 2. Verbesserungsliste

Kennzeichnung: **[S]** = nur der Shinki-Look (Ausprägung aus dem Clickdummy), **[A]** = alle Stile
(Struktur und Vermittlung, die Ausprägung ergibt der Look des Stils).

### A. Kopf und Navigation [A]

1. **Pfad-Breadcrumb statt Zurück-Spalte.** Im Shinki-Look zeigt die App oben einen
   Kirigami-Pfad („Mitori 見取り › Einstellungen › Darstellung", Screenshot). Vorlage und Bibel
   wollen eine rote Zurück-Spalte „‹ PROGRAMME" **ohne** Pfad (`ShinkiHead.qml:94-120` ist bereits
   da). → Prüfen, warum der Kirigami-Header trotz `globalToolBar.style: None` (`Main.qml:39`)
   sichtbar ist, und die Kopfzeile auf ShinkiHead festnageln.
2. **Eigene untere Leiste statt Aktionen in der Kopfzeile.** Suche und Zahnrad sitzen unten
   (Screenshot, blau = Material-Controls). CD: Zahnrad oben rechts, Icon-Buttons in der Kopfzeile
   (38 px, rund). → Untere Leiste weg, Aktionen in die Kopfzeile.
3. **Startmenü: Logo fehlt.** CD-Kopf hat das rote 神氣-Quadrat (40 px) links vor „Mitori 見取り".
   → Im ShinkiHead ergänzen.
4. **Startmenü: Raster.** CD ist ein 2-Spalten-Raster (`index.html:46`), die App stapelt auf dem
   Telefon einspaltig (`StartPage.qml:84`, `columns: wide ? 4 : 1`). → 2 Spalten auf dem Telefon.
5. **Startmenü: fehlende Elemente.** CD hat Hinweisboxen (App-/Video-Update, vorgemerkte Prüfung)
   über den Kacheln und einen Stand-Balken in der Programm-Kachel. Die App hat nur den
   Bundle-Update-Hinweis (`StartPage.qml:66`) und keinen Stand. → Update-/Prüfungs-Hinweise und
   den Stand ergänzen.

### B. Typografie und Textrollen [S]

6. **Technikname in der Serifenschrift.** CD-Listenzeile: Name in Crimson Pro 600, 16 px
   (`index.html:89`). Die App setzt Sans (Screenshot „Kamae-undo", „Mae-kaiten"). → Eine
   Look-Rolle `Look.listTitleFont` (Serif) einführen und in `LookListRow`/`LookTile` nutzen.
7. **Kommentar-Fließtext in Serif.** CD: 16 px Crimson Pro, Zeilenhöhe 1,5, Fußnoten mit 2-px-Linie
   links (`index.html:128-132`). `CommentPage.qml` nutzt QQC2.Label in Standardschrift. → Serif-Rolle
   + Fußnoten-Linie.
8. **Fünf Textrollen fehlen im Look.** CD hat genau fünf Rollen: Kicker (PT Sans Narrow 700, 11 px,
   gesperrt, rot), Titel (Serif 600, 22-30 px), Fließtext (Serif 400, 16 px), UI (PT Sans 13-14 px),
   Meta (11-12,5 px grau). `Look.qml` bietet nur Farb- und FONT-Rollen, keine Größenrollen, die
   Seiten greifen zu `Look.unit(px)` und Magic Numbers. → Größenrollen in `Look.qml` +
   `+shinki/Look.qml` ergänzen (die ShinkiStyle hat sie schon: `pageTitleSize`, `uiSize`,
   `listRowSize` …).
9. **Japanisch als Nebensatz, nicht vermischt.** CD: Kanji immer nach dem deutschen Titel, eigene
   Schrift (Noto Serif JP), grau. `ShinkiHead` kann das (`split()`), die Listenzeile der App
   nicht durchgängig. → Rolle `Look.japaneseFont` in Listen und Karten erzwingen.

### C. Listen, Karten, Zeichen [S]

10. **Stand als vier runde Punkte statt vier Quadrate.** CD: 8 px runde Punkte, gefüllt = rot
    (`index.html:92-94`); die App zeigt leere Kästchen (Screenshot). → Punktform statt Kästchen.
11. **Play-Knopf rot und rund.** CD: 34-px-Kreis, rot, weißes Dreieck (`index.html:95`). App:
    schwarzes Dreieck (Screenshot). → Rolle `Look.accentColor` im Play-Knopf.
### D. Player und Querformat [S]

14. **Transportleiste an die Vorlage.** CD: 5 Tasten, Pause 52 px rot in der Mitte, Tempi als
    Zeile „TEMPO 0,25 0,5 0,75 1×", Toggles als drei gleich breite Knöpfe. App: `LookTransportBar`
    (Kante-Optik). → Kante-Transportleisten-Baustein um die CD-Form erweitern.
15. **Querformat-Schiene.** CD: 62 px dunkle Schiene rechts, Video `calc(100% - 62px)`, Zeitleiste
    unten über die volle Breite. App: 2 Spalten über die ganze Höhe, keine schmale Schiene, keine
    Zeitleiste über die Breite. → Nachbauen (PlayerView `landscape`).
16. **Doppeltipp-Anzeige.** CD zeigt beim Tippen „2 s ››" / „‹‹ 2 s" auf der getippten Hälfte
    (`index.html:149-150`). App springt, zeigt aber nichts. → Anzeige ergänzen.
17. **Bewertung und Notiz im Player.** CD: Karte „Wie gut sitzt es?" + Notizzeile mit Eingabefeld und
    rotem Senden-Knopf. App hat `RatingRow`/`NoteRow` ✓ – Optik (Serif-Eingabefeld, roter Knopf)
    angleichen.

### E. Look-Layer und Shinki-Vollständigkeit [A]

18. **Shinki hat keinen Controls-Style.** `appearance.cpp:74-88`: Shinki fällt auf Breeze/Material
    zurück („until then Kante's controls"). Folge: alle direkt benutzten QQC2-Controls bleiben
    **gerundet und blau** (Screenshot: untere Leiste blau) – der einzige Farbbruch im Look.
    → Shinki-eigene Controls (Papier, eckig, rote Akzente) in `shinki/qml/Shinki/` bauen und in
    `controlsStyle()` eintragen.
19. **Direkte QQC2/Kirigami-Nutzung umstellen.** ~60 Stellen in 20 Seiten nutzen QQC2.Label,
    QQC2.ScrollView, QQC2.Control, QQC2.ToolBar, QQC2.Menu, QQC2.Overlay, `Kirigami.FormLayout`,
    `Kirigami.PlaceholderMessage`, `Kirigami.Separator` statt Look-Bausteinen. → Für die
    Look-Bausteine fehlen: `LookLabel`, `LookField` (Textfeld), `LookTextArea`, `LookMenu`,
    `LookScrollView`, `LookEmptyState`, `LookFormLayout`, `LookToolbar` (vorhanden sind 36
    Look-Bausteine – die Formular- und Menü-Bausteine fehlen).
20. **Landkarte ohne Shinki.** `TechniqueMapPage.qml:18` nutzt `KanteScope` direkt; es gibt kein
    `LookHeatGrid` und kein `ShinkiHeatGrid`. → Look-Baustein für das Raster bauen.
21. **`Look.qml` kennt nur 20 Farbrollen.** Fehlend u. a. Chip-Fläche, „ausgewählte Zeile",
    Scrim, Text auf Rot, Spalten-Tracking. → ergänzen, sonst weichen die Seiten aus.
22. ~~**KanteStyle unter Shinki.**~~ → steht in Abschnitt 3 (Trennung der Themes), Punkt 1 und 5.
23. **Leere Zustände.** CD hat Leerzustände (`.ph` mit 後 als Platzhalter); App nutzt
    `Kirigami.PlaceholderMessage` (Desktop-Optik) oder gar nichts. → `LookEmptyState` bauen.

### F. Formulare und Bedienung [A]

24. **Formulare im Kirigami-Look.** `SettingsPage.qml:22` und `CourseNewPage.qml:45` nutzen
    `Kirigami.FormLayout` mit `Kirigami.FormData.label` – Feldlabels in Kirigami-Schrift,
    Zeilenabstände und Fokus wie am Desktop. CD: Label als gesperrter roter Kicker, Felder 1 px
    gerahmt, Fläche `#faf9f7`. → Look-Formularzeile bauen.
25. **Eingabefelder.** CD: 1 px Rahmen, helle Fläche, Serif 15 px, Fokus rot (`.qnote input`).
    App: QQC2-Controls (gerundet, Material). → mit Punkt 18/19 lösen.
26. **Bestätigungsdialoge.** `ExamEndPage.qml:82`, `CoursePage.qml:453`:
    `standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel` – Plattform-Buttons, nicht Look.
    → Look-Dialog mit CD-Buttons (rot gefüllt, eckig, ohne Radius).

### G. Tests und Pflege [A]
---

## 3. Trennung der Themes: Shinki ≠ Kante

Shinki (`shinki/tokens.json`, 35 eigene Bausteine) und Kante (`app/qml/Kante/`, eigene Palette
und eigene Bausteine) sind **zwei Themes**, keine Varianten. Jede Seite liest eine Rolle; die Rolle
liefert der Look des laufenden Stils. An fünf Stellen ist diese Trennung noch nicht sauber:

1. **`Main.qml:32` – `KanteScope { target: root.contentItem }` für das ganze Fenster,** unabhängig
   vom Stil. Unter Shinki steht damit die Kante-Palette im Fensterrahmen. → Den Scope wie die
   Bindings in `Main.qml:17-31` nur setzen, wenn der laufende Stil Kante/Leinen ist.
2. **`TechniqueMapPage.qml` ist eine Kante-Seite** (`import Kante`, `KanteScope:18`,
   `KanteHeatGrid:106/139`, `KanteChip`, `KanteSegmented`, `KanteSectionLabel`, `KanteStyle.*`).
   Im Shinki-Look zeigt die Landkarte damit Kante. → Look-Bausteine bauen
   (`LookHeatGrid`, `LookChip`, `LookSegmented`, `LookSectionLabel`) mit Shinki-Variante.
3. **`appearance.cpp:74-88` (`controlsStyle`) liefert für Shinki „Material"** (oder Breeze), der
   Kommentar sagt „until then Kante's controls". Das ist ein **dritter** Look, den es nicht gibt –
   daher die blaue, gerundete Leiste im Shinki-Screenshot. → Entweder Shinkis eigene Controls
   (ShinkiButton, ShinkiComboBox, ShinkiTextField, ShinkiSwitch, ShinkiDialog, ShinkiToast liegen
   schon als QML vor) zu einem Controls-Style zusammenführen, oder Material ausdrücklich als
   Plattform-Fallback benennen und den Kommentar richtig setzen. Was **nicht** geht: die Shinki-
   Seiten in Kante-Optik zu tauchen.
4. **`Main.qml:14`** – der Kommentar „Shinki stands in Leinen until app/qml/Look reaches it" ist
   veraltet: `app/qml/Look` ist da, Shinki hat 35 eigene Look-Bausteine.
5. **`Look.headingFont()` / `Look.monoFont()` / `Look.unit()`** lesen im Basisfall `KanteStyle`
   (`app/qml/Look/Look.qml:39-52`). Im Shinki-Fall müssen sie `ShinkiStyle` liefern
   (`shinki/tokens.json`: `kpi` 28, `listRow` 16, `gridUnitPx` 18). → Die Basisrollen in
   `Look.qml` und `+shinki/Look.qml` je Theme füllen, nicht nur die Farben.

Das Muster, das trägt, steht schon an einer Stelle richtig: `app/qml/Look/Look.qml` liest
`KanteStyle`, `app/qml/Look/+shinki/Look.qml` liest `ShinkiStyle` – dieselben Rollennamen, zwei
Welten. Fehlend ist nur `LookFocusFollow` (verhaltens-only, kein visuelles Problem).

---

## 3a. Stand der Umsetzung

Die Trennung (Abschnitt 3) und der Shinki-Abgleich (Abschnitt 2, [S]-Punkte) sind angefangen;
dies ist der Stand vom 03./04.11.2026.

**Trennung – erledigt**
- Der Scope des Fensters folgt dem Stil (`Main.qml`: `LookScope` statt `KanteScope`), die
  Kante-Bindungen nur unter Kante und Leinen.
- Die Landkarte läuft über Look-Bausteine (`LookHeatGrid` neu: Look-Farben, Look-Schrift,
  Tastatur auf der Zelle, `rowCount`/`columnCount`/`cellAt`/`entryOf`/`isEmpty`).
- Neue Look-Rollen, je Stil getrennt: `chipColor`, `selectedColor`, `scrimColor`,
  `onAccentColor`, `levelTextColor`, `readingFont`, `sheetTitleFont`, `narrowFont`,
  `columnTracking`.
- Die Direktnutzung von QQC2/Kirigami in den Seiten ist weg: 108 Labels → `LookLabel`,
  `Kirigami.FormLayout` → `LookFormRow`, ScrollViews, Toolbars, Menüs, TextArea und
  Leerzustand → eigene Look-Bausteine.
- Die Tablet-Erkennung hängt an der Höhe in dp (460), nicht an der Schrift: im Shinki-Look ist
  die Schrift größer, die alte Bedingung (`gridUnit * 26`) unterschied das Gerät nicht.

**Shinki – erledigt**
- Das Logo (神氣) in der Kopfzeile, der Gruß auf der Startseite, das Raster der Kacheln
  (zwei Spalten auf dem Telefon, die Programm-Kachel über die ganze Breite) und der Stand in
  der Programm-Kachel („3 von 8 prüfungsreif").
- Der Kommentar liest in der Leseschrift des Stils (im Shinki-Look Crimson Pro).
- Das Technikblatt ist im Shinki-Look ein Blatt von unten (Griff, Titel in der Buchschrift,
  eckig, volle Breite); im Kante-Look bleibt der Dialog.
- Die Schalter des Players (Ton, Bildschirm) sind eckige Felder mit einem Rahmen in der
  Akzentfarbe statt runder Kante-Knöpfe.
- Der Doppeltipp zeigt, was er getan hat („2 s ››" auf der getippten Hälfte).

**Noch offen**
- Die Querformat-Schiene des Players (eine schmale Leiste rechts statt der breiten
  Controls-Spalte) – CD-Punkt 15.
- Shinkis eigener Controls-Style (die Plattform-Controls bleiben gerundet und blau, wo eine
  Seite sie noch direkt benutzt) – CD-Punkt 18.
- Die Fortschrittskarte der Programmseite („Prüfungsreif x/y" mit dem 5-Teil-Balken): der
  Balken ist da (`GradeStand`), der Text fehlt noch.
- Die Prüfliste (Abschnitt 4) ist noch nicht am Stück geprüft: die Punkte 1-20 sind einzeln
  abzuhaken, am besten mit den Screenshots des UI-Tests (`build*/ui-shots/shinki/`).

## 4. Prüfliste Shinki (aus dem Clickdummy, zum Abhaken)

Diese Liste beschreibt **nur den Shinki-Look**. Für Kante gilt die Kante-Bible, nicht diese Liste –
die beiden Stile dürfen und sollen unterschiedlich aussehen.



A. Grundsatz – Ruhe vor Effekt
1. Keine Verläufe, keine Schlagschatten, keine Rundungen: Karten, Zeilen, Chips, Buttons, Felder
   sind eckig. Nur Play-, Pause-, Icon- und Punktknöpfe sind rund.
2. Genau **eine** Akzentfarbe (#ce171e) in der ganzen App.
3. Farbe trägt Bedeutung: dieselbe 5-Stufen-Skala für Standpunkte, Fortschrittsbalken, Heatmap,
   Landkarten-Matrix.
B. Papier & Tusche
4. Hintergrund weiß, zweite Fläche warmes Off-White (#faf9f7), Chip-Fläche #f3f1ee.
5. Text fast Schwarz (#1d1b1a), Nebentext warmes Grau (#77716c).
6. Dünne Linien trennen statt Kästen: 1 px #e6e2dd, 3 px rote Innenkante nur für „ausgewählt".
7. Bildzeichen sind japanische Kanji in Rot (神氣, 技, 稽, 試, 記, 講, 図, 辞, 映, 保, 色).
C. Typografie-Hierarchie
8. Fünf Textrollen, überall gleich benannt: Kicker → Titel → Fließtext → UI → Meta.
9. Japanisch immer als Nebensatz in eigener Schrift, nach dem deutschen Titel.
10. Zahlen durchgängig tabellarisch, Dezimalkomma statt Punkt.
11. Versalien und Sperrung nur für Kleines, nie für Fließtext oder Titel.
D. Zeilen und Listen
12. Listenzeile = Name (Serif 600, 16 px) + Stand (4 Punkte, 8 px) + Play-Kreis (34 px, rot),
    feste 3-Spalten-Rasterung; Hover nur als Hintergrundton.
13. Drei Listenebenen: Gruppenkopf (roter Kicker mit Unterlinie), Angriffszeile (13 px Sans),
    Technikzeile.
14. Karten: weiß, 1 px Rahmen, 12-14 px Innenabstand, Kopf als roter Kicker; kein Schatten.
E. Bedienung
15. Primärbutton rot, weiß, fett, vollflächig, ohne Radius; Sekundärbutton weiß mit 1 px Rahmen;
    in Listen und Kopfzeilen nur Icon-Buttons (38 px, rund).
16. Jede Ansicht hat genau **eine** zurück-Zeile in der Kopfzeile, **keinen** Pfad-Breadcrumb,
    keine Tab-Leiste; rechts optional 1-3 Icon-Buttons.
17. Mehrfachauswahl als Segmentierung (Sparte) oder Chip-Leiste (Grade), nie als Dropdown;
    Schalter als eigene 42 × 24.
18. Ein-, Zwei- und Vierauswahlwerte immer dasselbe Muster: gleich breite Felder, große Ziffer,
    Wort darunter, aktives Feld rot gefüllt.
19. Status- und Fehlermeldungen als Hinweiszeile mit 3 px roter Linkkante, nie als rote Vollfläche.
F. Ruhe und Bewegung
20. Keine Animation außer dem kurzen Aufleuchten der zuletzt geöffneten Listenzeile (1,6 s) und
    dem Ein-/Ausblenden von Overlays; Touch-Ziele ≥ 34 px; Seitenrand 16 px (Tablet 28 px).

---

## 5. Offene Fragen

- **Wie weit wandert der Look?** Punkt 19 (QQC2/Kirigami direkt) betrifft ~60 Stellen in 20 Seiten.
  Das ist eine Etappe für sich, kein nebenbei.
- **Wie entstehen Shinkis Controls?** Shinki hat 35 QML-Bausteine, aber keinen
  Qt-Controls-Style (`QQuickStyle`). Ohne ihn bleiben die Plattform-Controls (gerundet, blau) –
  Punkt 3 in Abschnitt 3. Braucht Shinki einen eigenen Controls-Style, oder bleiben die
  Plattform-Controls und die Shinki-Seiten steuern sie nur an?
- **Welche der Punkte 1-29 sind Termine?** Der `ROADMAP.md` führt die offenen Punkte; dieser
  Vergleich passt als Zeile in Schritt 3 („Prüfen und Lernen") oder als eigener Schritt
  („Look: Shinki fertig machen").


27. **Keine UI-Tests für die Shinki-Varianten.** `app/tests/qml/tst_ui.qml` prüft Look-Bausteine
    allgemein, aber keine der 35 `+shinki`-Dateien einzeln. → je Shinki-Baustein ein Fall
    (der Oberflächentest legt Bilder nach `<build>/ui-shots`).
28. **`LookPageTitle` ungenutzt**, `Look.titleFont()` nur 2× (`ExamBoard.qml:24`,
    `WelcomePage.qml:116`) – die Seitentitel laufen über `LookHead`. → entweder nutzen oder
    entfernen (ShinkiStyle hat `pageTitleSize`, das ist die Lücke).
29. **Screenshots als Referenz.** Der Clickdummy ist die Abnahme-Vorlage; die App braucht denselben
    Blick pro Ansicht. → Die 13 CD-Ansichten als Testfälle in `tst_ui.qml` aufnehmen (Bilder je
    Seite, hoch und quer).

12. **Gruppenkopf als roter Kicker mit Unterlinie.** CD `.gh` (gesperrt, rot, 1 px Linie darunter).
    App: „KATA 型" – Form prüfen, Farbe/Tracking über `LookSectionLabel` führen.
13. **Kartenabstände aus dem Look, nicht aus Kirigami.Units.** CD-Karten: 1 px Rahmen,
    12-14 px Innenabstand, kein Schatten, keine Rundung (`index.html:47`). Die App setzt
    `Kirigami.Units.largeSpacing`. → Abstände als Look-Rollen führen.
