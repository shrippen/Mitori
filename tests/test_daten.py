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


def test_uebersetzungen_vollstaendig():
    """Every text of the app has its German translation (app/i18n/*.ts)."""
    import xml.etree.ElementTree as ET
    for ts in (WURZEL / "app" / "i18n").glob("*.ts"):
        offen = [m.findtext("source") for m in ET.parse(ts).iter("message")
                 if m.find("translation").get("type") == "unfinished"
                 or not "".join(m.find("translation").itertext()).strip()]
        assert not offen, f"{ts.name}: {offen}"
