# Repo-Aufteilung — Koordination mit dem parallelen Agent

Stand: 04.11.2026. Mitori ist in drei Repos aufgeteilt:

| Repo | Pfad | Submodul-Quelle |
|---|---|---|
| `mitori-daten` (neu, lokal unter `../mitori-daten`) | Daten, Quellen, Werkzeuge, Video-Bundle-Encoder, Shinki-Stil | – |
| `mitori-heft` (neu, lokal unter `../mitori-heft`) | das gedruckte Heft | `mitori-daten` als Submodul |
| `Mitori` (App) | die Qt-App | `mitori-daten` als `third_party/mitori-daten/` |

## Was im App-Repo fehlt

- `daten/`, `quellen/`, `tools/` (Datenwerkzeuge), `shinki/` (komplett), `heft/`, `KORREKTUREN.md`
- `tests/test_daten.py`, `tests/test_bundle.py`, `tests/test_shinki.py` — leben jetzt im motori-daten-Repo

## Wo dieselben Pfade jetzt stehen

| Pfad vorher | Pfad jetzt |
|---|---|
| `python3 tools/daten.py` | `python3 third_party/mitori-daten/tools/daten.py` |
| `${MITORI_REPO}` in `app/CMakeLists.txt` | `${MITORI_DATA_DIR}` (Default `third_party/mitori-daten`) |
| `shinki/qml/Shinki/*` | `third_party/mitori-daten/shinki/qml/Shinki/*` |
| `quellen`, `KORREKTUREN.md`, `daten/*.yaml` | im Submodul |

## Shinki als getrenntes QML-Modul

`app/CMakeLists.txt` baut ein eigenes Library-Target `mitori_shinki` (URI
`Shinki`) aus den Files im Submodul. `mitori_app` linkt PRIVATE darauf.
Jede QML-Datei bekommt `QT_RESOURCE_ALIAS=<basename>` (Qt kann absolute
Pfade nicht selbst auflösen).

## CI geändert

- `actions/checkout@v7` mit `submodules: recursive` (in `checks.yml`,
  `android.yml`, `release.yml`).
- `checks.yml` ruft zusätzlich `cd third_party/mitori-daten && pytest &&
  daten.py && kommentare.py && lexikon.py` auf (Daten-Tests im Submodul).

## Was der parallele Agent beachten muss

- Branch: die Änderungen liegen in `split/data` ab `origin/main`. Bevor
  die Look-Änderungen gemergt werden, muss `split/data` in main gemergt
  sein, oder die Look-Änderung rebased auf `split/data`.
- Unversionierte Dateien im Hauptverzeichnis (`app/qml/Look/Look*.qml`)
  müssen gegen den Merge keine Konflikte bekommen — sie sind im
  Worktree nicht vorhanden.
- Der `ui_shinki`-Test (ctest 20) segfaultet auf `origin/main`; das hat
  nichts mit der Migration zu tun, sondern fehlenden Look-Komponenten.
  Wer den Bug fixt, fixt nicht die Migration.

## Was beim Push zu tun ist

`mitori-daten` und `mitori-heft` müssen erst auf Gitea angelegt
werden; dann in `.gitmodules` den URL auf
`https://git.arianw.de/shrippen/mitori-daten.git` (bzw. `mitori-heft`)
umstellen.
