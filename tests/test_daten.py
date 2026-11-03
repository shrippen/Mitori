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


def test_kommentare_mit_titel_und_text(daten):
    """Every comment the app shows is {titel, text} (CommentPage reads both): Kiriotoshi too."""
    def kommentare(x):
        if isinstance(x, dict):
            if "kommentar" in x:
                yield x.get("id"), x["kommentar"]
            for v in x.values():
                yield from kommentare(v)
        elif isinstance(x, list):
            for v in x:
                yield from kommentare(v)
    falsch = [i for i, k in kommentare(daten) if not (isinstance(k, dict) and k.get("titel") and k.get("text"))]
    assert not falsch, falsch


def test_kamae_eine_technik(daten):
    """Die Kamae-Übung ist ein Eintrag (ein Ablauf); die 26 Positionen sind seine Beschreibung (S-09, V-13)."""
    kamae = daten["disziplinen"][2]["kamae"]
    assert kamae["id"] == "shinkitoho/kamae"
    assert (kamae["name"], kamae["kanji"]) == ("Kamae-Übung", "構")
    namen = [p["name"] for p in kamae["positionen"]]
    assert len(namen) == 26 and namen.count("Seigan") == 2
    assert all(p["kanji"] for p in kamae["positionen"])
    # Der Kommentar zählt wie das Blatt (1–26), nicht wie die Quelle (1–24).
    text = kamae["kommentar"]["text"]
    assert "\n\n2–3. Battô Seigan\n" in text and "\n\n4. " in text and "\n\n26. " in text
    assert kamae["video"] == {"video": "shinkitoho/suburi", "von": 212.88, "bis": 593.3}
    assert not any(i.startswith("shinkitoho/kamae/") for i in alle_ids(daten))


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


def test_glossar_ohne_dubletten(daten):
    """One word per term: lexicon alternatives in brackets (-jime) and oo as o join begriffe.yaml."""
    import importlib.util
    spec = importlib.util.spec_from_file_location("daten", WURZEL / "tools" / "daten.py")
    modul = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(modul)
    woerter = daten["glossar"]["woerter"]
    schluessel = [modul.glossar_schluessel(modul.stichwort(w["wort"])) for w in woerter]
    doppelt = sorted({s for s in schluessel if schluessel.count(s) > 1})
    assert doppelt == ["ha", "ho", "kata"]            # homonyms with different meanings
    nach_wort = {w["wort"]: w for w in woerter}
    assert nach_wort["Oo-guruma"]["jp"] == "大車"
    assert nach_wort["Kubi-shime (-jime)"]["jp"] == "首締め"
    assert nach_wort["Ryote-tori (-dori)"]["jp"] == "両手取り"
    assert "O-guruma" not in nach_wort and "Kubi-jime" not in nach_wort


def test_glossar_ohne_offene_kanji(daten):
    """A term whose kanji is an open question (KORREKTUREN.md, status klären) stays out: A-K-04."""
    assert not [w for w in daten["glossar"]["woerter"] if w["wort"] == "Shiho-gaeshi"]


def test_glossar_lesung_an_wortgrenzen(daten):
    """Joined words read at the boundary of known words: Yokomenuchi as yokomen + uchi."""
    nach_wort = {w["wort"]: w for w in daten["glossar"]["woerter"]}
    assert nach_wort["Yokomenuchi"]["kana"] == "よこめんうち"


@pytest.mark.parametrize("wort, lesung", [
    ("Kiri-otoshi", "きりおとし"), ("Shomen-uchi", "しょめんうち"), ("Ikkajô", "いっかじょう"),
    ("San'nin-gake", "さんにんがけ"), ("Shikkoh", "しっこう"), ("Massugu", "まっすぐ"),
    ("Kime (-gime)", "きめ"), ("Yame!", "やめ"), ("Jo-suburi 2", "じょすぶり　に"), ("Ryote-tori", "りょてとり"),
    ("Sempai", "せんぱい"), ("Lex", None),
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


def test_englisch_nicht_deutsch():
    """mitori_en.ts holds English plural forms: none of them is the German one of the same text."""
    import xml.etree.ElementTree as ET
    i18n = WURZEL / "app" / "i18n"
    deutsch = {m.findtext("source"): {f.text for f in m.iter("numerusform")}
               for m in ET.parse(i18n / "mitori_de.ts").iter("message")}
    falsch = [m.findtext("source") for m in ET.parse(i18n / "mitori_en.ts").iter("message")
              if {f.text for f in m.iter("numerusform")} & deutsch.get(m.findtext("source"), set())]
    assert not falsch, f"mitori_en.ts has German forms: {falsch}"


# Waiting times (Meine Grade): every grade one can take has `wartezeit` {jahre, nach} from its
# `abstand`: "nach" is "grad" (the previous grade) or "beginn" (the start of training).
WARTEZEITEN = {
    "aikido": [None, 1, 1, 1, 1, 1, 2, 3, 4],
    "daitoryu": [None, 1, 1, 1, 1, 1, 2, 3, 4],
    "shinkitoho": [2, 2, 3, 3, 4, 5],
}


def test_wartezeiten(daten):
    for d in daten["disziplinen"]:
        pruefbar = [g for g in d["grade"] if g.get("pruefbar", True)]
        jahre = [g["wartezeit"]["jahre"] if g.get("wartezeit") else None for g in pruefbar]
        assert jahre == WARTEZEITEN[d["id"]], d["id"]
        nach = [g["wartezeit"]["nach"] for g in pruefbar if g.get("wartezeit")]
        erwartet = ["beginn"] + ["grad"] * 5 if d["id"] == "shinkitoho" else ["grad"] * 8
        assert nach == erwartet, d["id"]
    hoshu = [g for g in daten["disziplinen"][2]["grade"] if g["id"] == "hoshu"]
    assert hoshu and hoshu[0]["pruefbar"] is False


def test_korrekturen_status_nach_kopfzeile(tmp_path):
    """The status comes from the column named Status, also with more columns after it (the owner's
    decisions)."""
    sys.path.insert(0, str(WURZEL / "tools"))
    import daten as werkzeug
    datei = tmp_path / "KORREKTUREN.md"
    datei.write_text(
        "| ID | Stelle | Status | Entscheidung vom Owner |\n"
        "|---|---|---|---|\n"
        "| A-K-01 | PO | klären | Als Frage an Sensei notieren |\n"
        "| A-K-02 | PO | erledigt | |\n",
        encoding="utf-8")
    assert werkzeug.korrekturen(datei) == {"A-K-01": "klären", "A-K-02": "erledigt"}
