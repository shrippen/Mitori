# Mitori: QA-Journeys

Feste Journeys je Persona ([`PERSONAS.md`](PERSONAS.md)). Wer einen Ablauf ändert, spielt die betroffenen durch.

## Ablauf einer Sitzung

1. **Aufbau.** `qa/build.sh`; jede Sitzung in ihrem eigenen X-Server und Profil (`qa/session.sh`, Kopf lesen): sie sieht
   weder Desktop noch echtes Profil. Mit `--keep DIR` geht die nächste Sitzung mit denselben Daten weiter.
2. **Als Persona spielen.** Gerät nachstellen (`--size`, `--scale`, `--style`, `--lang`), mit `tap`, `type_text`, `key`,
   `shot` aus `qa/lib.sh`, Bilder ansehen. Entscheidungen treffen wie die Persona, nicht wie ein Entwickler.
3. **Nebenher.** `warnings`: QML-Warnungen; dazu Sprachmix, abgeschnittene Texte, Farbe als einziges Signal, Fokus
   nicht sichtbar, Knöpfe zu klein, leere Seiten ohne Erklärung, Abstände, Vergleich mit dem Clickdummy.
4. **Fund notieren:** Stufe (Fehler, Reibung, unlogisch, kosmetisch), Persona, Journey und Schritt, was passierte gegen
   was erwartet war, Beleg. Nur Beobachtetes; Ungetestetes ausdrücklich nennen.
5. **Ergebnis** der Runde in `ROADMAP.md` unter „QA-Runde <Datum>“; Fehler: zuerst ein Test, der fehlschlägt.

**Videos bleiben privat:** Bilder mit einem Videobild verlassen den Rechner nie (`agent.md`).

## Journeys

**J-A1 Erster Start** · P1, P6. `--new`: Assistent, Stil wählen, Rundgang, Startmenü.
Erwartet: unter einer Minute zum Startmenü, jeder Begriff erklärt, zurück jederzeit möglich.

**J-A2 Ohne Videos** · P6. `--no-videos --lang en`: Programm, Technik, Video, Einstellungen → Videos, Code eingeben (falsch).
Erwartet: sagt, warum nichts läuft und was zu tun ist; alles auf Englisch; ein falscher Code hat eine klare Meldung.

**J-B1 Technik nachschlagen** · P1, P2. Startmenü → Programm → Grad → Technik → Blatt → Video.
Erwartet: am Telefon höchstens vier Tipper; Stand je Grad sichtbar; zurück an dieselbe Stelle der Liste.

**J-B2 Player** · P1, P2, P4. Abspielen, Pause, Zeitlupe, Einzelbild, Schleife, Doppeltipp, Wischen, Vollbild ⤢ und zurück, drehen.
Erwartet: alle Knöpfe mittig und erreichbar, Ton in Zeitlupe aus, Name der Technik immer sichtbar, Stelle bleibt.

**J-B3 Einschätzen und Notiz** · P1, P4. Im Blatt und im Player einschätzen; Notiz schreiben, Foto, Sprachnotiz; Neustart.
Erwartet: bleibt nach einem Neustart; die Eingabe bleibt beim Tippen sichtbar.

**J-C1 Heute üben** · P1. Heute üben → Training starten → einschätzen bis fertig → Statistik.
Erwartet: klar, was als nächstes dran ist; am Ende ein Abschluss; der Tag zählt.

**J-D1 Prüfung** · P1, P5. Probeprüfung, Runden, abbrechen, zu Ende, Auswertung.
Erwartet: kein Datenverlust beim Abbruch; Ansage und Tafel lesbar; Techniken aus früheren Graden dabei.

**J-E1 Daten stimmen** · P5. Stichprobe je Disziplin gegen `third_party/mitori-daten/daten/*.yaml` und das Heft: Reihenfolge,
Namen, Kanji, Mindestabstand, Grade, Glossar, Landkarte.
Erwartet: keine Abweichung; offene Fragen als solche erkennbar.

**J-F1 Lehrgang** · P4. Lehrgang ohne Kalendereintrag anlegen, Notiz, Foto, Sprachnotiz, Export; alle Notizen.
Erwartet: schnell notiert, nichts geht verloren, Export lesbar.

**J-G1 Suche und Glossar** · P1, P2, P5. Suche mit und ohne Sonderzeichen („Ô-dachi“, „odachi“, „Kote“), Glossar, Aussprache.
Erwartet: trifft auch ohne Längungszeichen; Programm und Glossar getrennt, jeweils mit Zahl.

**J-H1 Tablet am Mattenrand** · P2. 1280×800 quer: Programm und kleiner Player, Technik wechseln, Vollbild, Heute, Lehrgänge, Statistik.
Erwartet: Liste und Player nebeneinander; große Ziele; Vollbild in einem Tipp.

**J-I1 Sehschwäche** · P3. `--scale 2`, Leinen: Startmenü, Programm, Blatt, Player, Prüfung, Einstellungen.
Erwartet: nichts abgeschnitten, nichts nur über Farbe, Kontrast genügt.

**J-I2 Nur Tastatur** · P7. Alle Seiten nur mit Tab, Enter, Leertaste, Esc, Pfeilen; Player mit Tasten.
Erwartet: Fokus immer sichtbar, jedes Element erreichbar, nichts fängt die Tastatur ein.

**J-J1 Sicherung** · P7. Export, Import (Ersetzen und Zusammenführen), Abgleich ohne Server, Konflikte, Darstellung wechseln, Sprache wechseln.
Erwartet: klare Meldung bei jedem Fehler; Daten bleiben; sagt, wo die Datei liegt.
