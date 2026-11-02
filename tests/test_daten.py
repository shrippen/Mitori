"""Checks of tools/daten.py and its output mitori-daten.json (the app's programme data)."""
import json
import subprocess
import sys
from pathlib import Path

import pytest

WURZEL = Path(__file__).resolve().parent.parent
FORMAT = 1  # FORMAT in tools/daten.py, Catalog::kFormat in app/catalog/catalog.h
DISZIPLINEN = ["aikido", "daitoryu", "shinkitoho"]


@pytest.fixture(scope="module")
def daten(tmp_path_factory):
    ausgabe = tmp_path_factory.mktemp("daten")
    subprocess.run([sys.executable, str(WURZEL / "tools" / "daten.py"), "--ausgabe", str(ausgabe)],
                   check=True, capture_output=True)
    return json.loads((ausgabe / "mitori-daten.json").read_text(encoding="utf-8"))


def alle_ids(daten):
    for d in daten["disziplinen"]:
        yield from (t["id"] for t in d.get("techniken", []))
        for art in ("kamae", "suburi", "odachi"):
            yield from (e["id"] for e in d.get(art, []))


def test_format_und_disziplinen(daten):
    assert daten["format"] == FORMAT
    assert [d["id"] for d in daten["disziplinen"]] == DISZIPLINEN


def test_ids_eindeutig(daten):
    ids = list(alle_ids(daten))
    doppelt = sorted({i for i in ids if ids.count(i) > 1})
    assert not doppelt


def test_techniken_verweisen_auf_bekannte_grade_und_gruppen(daten):
    for d in daten["disziplinen"]:
        grade = {g["id"] for g in d["grade"]}
        gruppen = {g["id"] for g in d.get("gruppen", [])}
        for t in d.get("techniken", []):
            assert t["grad"] in grade, t["id"]
            assert t["gruppe"] in gruppen, t["id"]


def test_jeder_grad_hat_stoff(daten):
    for d in daten["disziplinen"]:
        if "techniken" not in d:
            continue
        mit_stoff = {t["grad"] for t in d["techniken"]}
        assert {g["id"] for g in d["grade"]} == mit_stoff, d["id"]


def test_shinki_toho_kommentare(daten):
    st = daten["disziplinen"][2]
    assert all(k.get("kommentar") for k in st["kamae"])
    assert all(o.get("kommentar") for o in st["odachi"])


def test_glossar(daten):
    """Lexicon plus the terms of begriffe.yaml it lacks, alphabetical, each with a reading."""
    import yaml
    lexikon = yaml.safe_load((WURZEL / "daten" / "lexikon.yaml").read_text(encoding="utf-8"))
    glossar = daten["glossar"]
    woerter = glossar["woerter"]
    assert glossar["quelle"] == lexikon["quelle"]
    assert len(woerter) > len(lexikon["woerter"])
    assert {w["de"] for w in lexikon["woerter"]} <= {w["de"] for w in woerter}
    assert all(w["wort"] and w["kana"] and w["wort"][0].isupper() for w in woerter)
    namen = [w["wort"] for w in woerter]
    assert {"diagonal", "gerade", "Sonstige Techniken"}.isdisjoint(namen)
    kiri = [w for w in woerter if w["wort"] == "Kiri-otoshi"]
    assert kiri == [{"wort": "Kiri-otoshi", "jp": "切り落し", "de": "", "kana": "きりおとし"}]
    # Lexicon word without kanji: from begriffe.yaml, unless two words share the name (kata: form, shoulder).
    assert next(w for w in woerter if w["wort"] == "Shomen-uchi")["jp"] == "正面打ち"
    assert [w["jp"] for w in woerter if w["wort"] == "Kata"] == [None, None]
    assert namen.index("Ai-hanmi") < namen.index("Aikido") < namen.index("Zori")


@pytest.mark.parametrize("wort, lesung", [
    ("Kiri-otoshi", "きりおとし"), ("Shomen-uchi", "しょめんうち"), ("Ikkajô", "いっかじょう"),
    ("San'nin-gake", "さんにんがけ"), ("Shikkoh", "しっこう"), ("Massugu", "まっすぐ"),
    ("Kime (-gime)", "きめ"), ("Yame!", "やめ"), ("Jo-suburi 2", "じょすぶり　に"), ("Ryote-tori", "りょてとり"),
    ("Lex", None),
])
def test_kana(wort, lesung):
    import importlib.util
    spec = importlib.util.spec_from_file_location("daten", WURZEL / "tools" / "daten.py")
    modul = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(modul)
    assert modul.kana(wort) == lesung


def test_uebersetzungen_vollstaendig():
    """Every text of the app is in the German catalog and translated there (app/i18n/*.ts)."""
    import re
    import xml.etree.ElementTree as ET
    de = WURZEL / "app" / "i18n" / "mitori_de.ts"
    im_katalog = {m.findtext("source") for m in ET.parse(de).iter("message")}
    im_code = {t for qml in (WURZEL / "app" / "qml").rglob("*.qml")
               for t in re.findall(r'qsTr\("((?:[^"\\]|\\.)*)"', qml.read_text(encoding="utf-8"))}
    neu = sorted(im_code - im_katalog)
    assert not neu, f"not in mitori_de.ts (cmake --build <dir> --target update_translations): {neu}"
    for ts in (WURZEL / "app" / "i18n").glob("*.ts"):
        offen = [m.findtext("source") for m in ET.parse(ts).iter("message")
                 if m.find("translation").get("type") == "unfinished"
                 or not "".join(m.find("translation").itertext()).strip()]
        assert not offen, f"{ts.name}: {offen}"
