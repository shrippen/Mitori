# Mitori 見取り

App für Android und Plasma Mobile: Technik antippen, die Stelle im Prüfungsvideo läuft, in Zeitlupe
und als Schleife; danach abhaken und einschätzen. Prüfungsprogramme von Aikido Shinki Rengo,
Daitoryu und Itto-den Shinki Toho. Gedrucktes Heft im Schwester-Repo `mitori-heft`,
Programmdaten und Stil im Submodul `third_party/mitori-daten`.

Die Videos sind nicht Teil dieses Repos. Sie gehören zu den DVDs des Shinkiryu-Verbands und werden
in der App mit dem Code der DVD freigeschaltet.

## Aufbau

| Ordner | Inhalt |
|---|---|
| `app/` | die App (Qt 6, Kirigami, Design System Kante + Stil-Layer Look) |
| `entwuerfe/` | Konzept, Entscheidungen und Clickdummy der App |
| `packaging/` | Flatpak (`io.github.shrippen.Mitori.yml`), AppImage, metainfo, icons |
| `scripts/` | Build-Skripte: App, Android-App, AppImage, Flatpak, qmllint, Release-Notes, sync-kante |
| `shinki/` | der Stil Shinki: Bibel, Tokens, QML-Bausteine, Schriften |
| `tools/` | Werkzeuge von Shinki (Singleton, Schriften) |
| `third_party/mitori-daten/` | Submodul: Programmdaten, Quellen, Video-Bundle-Encoder |
| `tests/` | Python-Tests (Shinki, Übersetzungen, Android, Icons) |

Andere Repos:

| Repo | Inhalt |
|---|---|
| `mitori-heft` (Schwester) | gedrucktes Heft (LuaLaTeX) |
| `third_party/mitori-daten` (Submodul) | Daten, Quellen, Werkzeuge, Bundle-Encoder; Pin auf ein Datentag |

Plan: [ROADMAP.md](ROADMAP.md). Regeln für die Arbeit am Repo: [agent.md](agent.md).
Daten-Submodul: [mitori-daten](third_party/mitori-daten/README.md).

## Bauen

```sh
# Submodul beim ersten Klon:
git submodule update --init

./start.sh                                 # App bauen und starten, Videos aus ../Mitori Videos
cmake -S app -B build-app -G Ninja && cmake --build build-app && build-app/mitori
scripts/check.sh                           # alle Prüfungen wie in der CI
scripts/build-android.sh                   # APK → dist/android/
```

Daten prüfen und erzeugen läuft automatisch beim App-Build (CMake ruft
`third_party/mitori-daten/tools/daten.py` auf). Wer die Daten separat prüfen will:

```sh
python3 third_party/mitori-daten/tools/daten.py
```

## Lizenz

GPL-3.0-or-later. Texte und Listen aus `third_party/mitori-daten/quellen/` und die daraus
übernommenen Kommentare gehören ihren Verfassern (Shinkiryu-Verband).
