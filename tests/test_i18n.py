"""The app's texts: every qsTr in the catalogs and translated, and Shinki's font has their kanji."""
import pathlib
import re
import xml.etree.ElementTree as ET

from fontTools.ttLib import TTFont

ROOT = pathlib.Path(__file__).resolve().parent.parent
APP = ROOT / "app"
DATEN = ROOT / "third_party" / "mitori-daten"
I18N = APP / "i18n"
QSTR = re.compile(r'qsTr\("((?:[^"\\]|\\.)*)"')
JAPANESE = re.compile("[　-ヿ㐀-䶿一-鿿豈-﫿＀-￯]")


def test_every_text_is_translated():
    catalog = {m.findtext("source") for m in ET.parse(I18N / "mitori_de.ts").iter("message")}
    code = {t for qml in (APP / "qml").rglob("*.qml") for t in QSTR.findall(qml.read_text(encoding="utf-8"))}
    new = sorted(code - catalog)
    assert not new, f"not in mitori_de.ts (cmake --build <dir> --target update_translations): {new}"
    for ts in I18N.glob("*.ts"):
        unfinished = [m.findtext("source") for m in ET.parse(ts).iter("message")
                      if m.find("translation").get("type") == "unfinished"
                      or not "".join(m.find("translation").itertext()).strip()]
        assert not unfinished, f"{ts.name}: {unfinished}"


def test_english_is_not_german():
    german = {m.findtext("source"): {f.text for f in m.iter("numerusform")}
              for m in ET.parse(I18N / "mitori_de.ts").iter("message")}
    wrong = [m.findtext("source") for m in ET.parse(I18N / "mitori_en.ts").iter("message")
             if {f.text for f in m.iter("numerusform")} & german.get(m.findtext("source"), set())]
    assert not wrong, f"mitori_en.ts has German forms: {wrong}"


def test_shinki_font_has_every_japanese_character():
    # Data and Shinki are checked in mitori-daten; only here do both meet the app's own texts.
    files = [p for d in [APP, DATEN / "daten", DATEN / "shinki" / "qml"] for p in d.rglob("*")
             if p.suffix in {".yaml", ".qml", ".js", ".ts", ".cpp", ".h"}]
    chars = {c for p in files for c in JAPANESE.findall(p.read_text(encoding="utf-8", errors="ignore"))}
    font = TTFont(DATEN / "shinki" / "fonts" / "NotoSerifJP-Regular-Subset.ttf").getBestCmap()
    missing = sorted(c for c in chars if ord(c) not in font)
    assert not missing, f"{''.join(missing)}: mitori-daten, README, Schriftzeichen-Pipeline"
