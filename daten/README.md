# Programmdaten

Eine Quelle für das Heft (`heft/`) und die App Mitori.

| Datei | Inhalt |
|---|---|
| `aikido.yaml`, `daitoryu.yaml` | Prüfungsprogramme; Aufbau im Kopf von `aikido.yaml` |
| `shinki-toho.yaml` | Grade, Zulassung, Kamae, Suburi, Ô-dachi-Formen |
| `shinki-toho-kommentare.yaml` | Kommentare zu Kamae, Suburi, Kiriotoshi und Ô-dachi aus den PDFs von shinkiryu.de; erzeugt von `tools/kommentare.py`, nur für die App |
| `lexikon.yaml` | Übersetzungen aus dem Aiki-Budo-Lexikon von shinkiryu.de; erzeugt von `tools/lexikon.py`, für das Glossar der App |
| `begriffe.yaml` | Kanji für Aikido und Shinki Toho (gesichert) |
| `daitoryu-begriffe.yaml` | Kanji-Bausteine für Daitoryu (Vorschlag, ungeprüft) |
| `aikido-vorbemerkung.tex` | Vorbemerkung der Aikido-Prüfungsordnung |

## Ändern

1. YAML-Datei bearbeiten. Eine Technik ist eine Zeile wie `Koshi-nage (soto-tenkan)`;
   mit offener Frage `"Nikyo (2 Arten) [A-K-03]"` (in Anführungszeichen).
2. `python3 tools/daten.py` prüft die Daten (unbekannte Grade, Frage-IDs ohne Eintrag in
   `KORREKTUREN.md`, Tippfehler in Feldnamen) und schreibt `build/daten/`.
3. `heft/build.sh` (oder `heft/build.sh entwurf`) baut das Heft; es ruft Schritt 2 selbst auf.

Ist eine Frage geklärt: Daten ändern, `[ID]` entfernen, in `KORREKTUREN.md` den Status setzen.

## Ausgabe für die App

`build/daten/mitori-daten.json` (Feld `format` = Version): je Disziplin Grade, Gruppen und alle
Techniken einzeln mit fester ID (`aikido/kyu-grade/tachi-waza-mae-seme/katate-katate-dori-ai-hanmi/k5/ikkyo-1`),
Angriff, Kanji und offenen Fragen; Shinki Toho mit Formen, Kamae und Suburi.
Namen ohne Kanji stehen in `build/daten/fehlende-kanji.txt`.
