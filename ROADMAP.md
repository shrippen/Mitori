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
| 4 · Bundles und Termine | `tools/bundle.py` (Beschnitt, Verschlüsselung, Manifest), Code-Eingabe und QR, Download; Lehrgänge aus ICS, Meine Grade, Wartezeiten, Lehrgangsnotizbuch, Sicherung (Export, Auto Backup, WebDAV-Abgleich) | ein Bundle per Code auf einem fremden Telefon geladen |
| 5 · Release | Demo-Modus mit Studio-Weber-Daten, Update-Check, Screenshots, Landing Page `docs/`, GitHub-Release | v1.0.0 auf GitHub und der Landing Page |

## Stand Schritt 0

- Daten aller drei Sparten in `daten/`, geprüft von `tools/daten.py`; Heft baut (`heft/build.sh`).
- Kapitel zugeordnet: nur Aikido 5. Kyu (Probe für den Clickdummy). Offen: alle übrigen Videos.

## Querschnitt

- **Theme Shinki** (Farben der Shinki News) als Rollen in Kante, wählbar neben Kante.
- **Videos ausliefern**: beschnitten (Aikido `crop=720:372:0:64`, Daitoryu `crop=720:374:0:50`,
  Shinki Toho ohne), HEVC, als versionierte Bundles im generischen Gitea-Paket.
- **Übersetzungen** deutsch und englisch von Anfang an.
- **Später**: Stores (F-Droid, Flathub), weitere Statistik-Auswertungen.
