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
        for art in ("suburi", "odachi"):
            yield from (e["id"] for e in d.get(art, []))
        for art in ("kamae", "kiriotoshi"):
            if art in d:
                yield d[art]["id"]


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
    assert st["kamae"]["kommentar"]["text"].startswith("1. Rei\n")
    assert all(o.get("kommentar") for o in st["odachi"])


def test_kamae_eine_technik(daten):
    """Die Kamae-Übung ist ein Eintrag (ein Ablauf); die 26 Positionen sind seine Beschreibung (S-09, V-13)."""
    kamae = daten["disziplinen"][2]["kamae"]
    assert kamae["id"] == "shinkitoho/kamae"
    assert (kamae["name"], kamae["kanji"]) == ("Kamae-Übung", "構")
    namen = [p["name"] for p in kamae["positionen"]]
    assert len(namen) == 26 and namen.count("Seigan") == 2
    assert all(p["kanji"] for p in kamae["positionen"])
    assert kamae["video"] == {"video": "shinkitoho/suburi", "von": 212.88, "bis": 593.3}
    assert not any(i.startswith("shinkitoho/kamae/") for i in alle_ids(daten))


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
