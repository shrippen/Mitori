"""The app's texts: every qsTr in the catalogs and translated."""
import pathlib
import re
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parent.parent
APP = ROOT / "app"
I18N = APP / "i18n"
QSTR = re.compile(r'qsTr\("((?:[^"\\]|\\.)*)"')


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


def test_every_text_is_in_its_context():
    """qsTr looks per file: a text that moved to another file falls back to English without a word."""
    contexts = {c.findtext("name"): {m.findtext("source") for m in c.iter("message")
                                     if m.find("translation").get("type") not in ("vanished", "obsolete")}
                for c in ET.parse(I18N / "mitori_de.ts").iter("context")}
    missing = [f"{qml.stem}: {text}" for qml in (APP / "qml").glob("*.qml")
               for text in QSTR.findall(qml.read_text(encoding="utf-8"))
               if "\\" not in text and text not in contexts.get(qml.stem, set())]
    assert not missing, f"cmake --build <dir> --target update_translations: {missing[:8]}"
