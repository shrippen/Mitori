# Mitori 見取り

Prüfungsprogramme von Aikido Shinki Rengo, Daitoryu und Itto-den Shinki Toho: als Heft und als App
für Android und Plasma Mobile. Technik antippen, die Stelle im Prüfungsvideo läuft, in Zeitlupe und
als Schleife; danach abhaken und einschätzen.

Die Videos sind nicht Teil dieses Repos. Sie gehören zu den DVDs des Shinkiryu-Verbands und werden
in der App mit dem Code der DVD freigeschaltet.

## Aufbau

| Ordner | Inhalt |
|---|---|
| `daten/` | die Programmdaten (YAML), eine Quelle für Heft und App |
| `tools/` | `daten.py` prüft die Daten und erzeugt Heft- und App-Daten |
| `heft/` | das Heft (LuaLaTeX) |
| `app/` | die App (Qt 6, Kirigami, Design System Kante) |
| `quellen/` | Prüfungsordnungen und Listen, aus denen die Daten stammen |
| `entwuerfe/` | Konzept, Entscheidungen und Clickdummy der App |

Plan: [ROADMAP.md](ROADMAP.md). Regeln für die Arbeit am Repo: [agent.md](agent.md).

## Bauen

```sh
python3 tools/daten.py                 # Daten prüfen (braucht PyYAML)
heft/build.sh                          # Heft → heft/build/
cmake -S app -B build-app -G Ninja && cmake --build build-app && build-app/mitori
scripts/check.sh                       # alle Prüfungen wie in der CI
scripts/build-android.sh               # APK → dist/android/
```

## Lizenz

GPL-3.0-or-later. Die Texte in `quellen/` und die daraus übernommenen Kommentare gehören ihren
Verfassern (Shinkiryu-Verband).
