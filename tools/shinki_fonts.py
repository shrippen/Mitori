#!/usr/bin/env python3
"""Makes Shinki's fonts (shinki/README.md, section 3) for the app: shinki/fonts/*.ttf, each a subset.

  tools/shinki_fonts.py <folder with the fonts of google/fonts (ofl/…)>

  PT Sans Regular and Bold, PT Sans Narrow Bold
      unchanged: their OFL reserves the name "PT Sans", a subset (a Modified Version) may not keep it.
  Crimson Pro (variable, 200–900)
      Latin only (the app's languages are German and English): a third of the size.
  Noto Serif JP Regular (an instance of the variable font)
      only the kanji and kana the app shows: programme data (daten/), QML and catalogs (app/).
      The whole font has 13 MB; tests/test_shinki.py checks that the subset still covers
      every character, so a new kanji in the data fails until this tool runs again.

Needs fontTools (pyftsubset); the CI does not, it reads the committed subsets.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "shinki" / "fonts"

# Latin, Latin-1, Latin Extended-A/B, general punctuation, € and arrows (→ in texts).
LATIN = "U+0000-024F,U+02C6-02DC,U+2000-206F,U+20AC,U+2190-21FF,U+2212,U+2264-2265,U+25A0-25FF"
WHOLE_FONTS = {
    "PT_Sans-Web-Regular.ttf": "PTSans-Regular.ttf",
    "PT_Sans-Web-Bold.ttf": "PTSans-Bold.ttf",
    "PT_Sans-Narrow-Web-Bold.ttf": "PTSansNarrow-Bold.ttf",
}
LATIN_FONTS = {
    "CrimsonPro[wght].ttf": "CrimsonPro.ttf",
}
JAPANESE_FONT = "NotoSerifJP[wght].ttf"
JAPANESE_OUT = "NotoSerifJP-Regular-Subset.ttf"


def is_japanese(ch):
    """Kana, CJK ideographs and the CJK punctuation the app writes (「」・ー …)."""
    c = ord(ch)
    return (0x3000 <= c <= 0x30FF or 0x3400 <= c <= 0x4DBF or 0x4E00 <= c <= 0x9FFF
            or 0xF900 <= c <= 0xFAFF or 0xFF00 <= c <= 0xFFEF)


def japanese_chars():
    """Every Japanese character in the programme data, the QML and the catalogs."""
    files = list((ROOT / "daten").glob("*.yaml")) + list((ROOT / "app" / "qml").rglob("*.qml"))
    files += list((ROOT / "app" / "qml").rglob("*.js")) + list((ROOT / "app" / "i18n").glob("*.ts"))
    files += [p for p in (ROOT / "app").rglob("*.cpp")]
    chars = set()
    for path in files:
        chars |= {ch for ch in path.read_text(encoding="utf-8", errors="ignore") if is_japanese(ch)}
    return chars


def find(source, name):
    hits = [p for p in source.rglob("*") if p.name.endswith(name)]
    if not hits:
        sys.exit(f"shinki_fonts: {name} not in {source}")
    return hits[0]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    from fontTools import subset
    from fontTools.ttLib import TTFont
    from fontTools.varLib import instancer

    source = Path(sys.argv[1])
    OUT.mkdir(parents=True, exist_ok=True)
    for name, out in WHOLE_FONTS.items():
        (OUT / out).write_bytes(find(source, name).read_bytes())
    for name, out in LATIN_FONTS.items():
        subset.main([str(find(source, name)), f"--unicodes={LATIN}", "--layout-features=*",
                     f"--output-file={OUT / out}"])

    font = TTFont(find(source, JAPANESE_FONT))
    # The names of the Regular instance (else the default instance's: "Noto Serif JP ExtraLight").
    regular = instancer.instantiateVariableFont(font, {"wght": 400}, updateFontNames=True)
    full = OUT / "NotoSerifJP-Regular.tmp.ttf"
    regular.save(full)
    text = "".join(sorted(japanese_chars()))
    subset.main([str(full), f"--text={text}", "--layout-features=*", f"--output-file={OUT / JAPANESE_OUT}"])
    full.unlink()

    for licence in sorted(source.rglob("OFL.txt")):
        (OUT / f"OFL-{licence.parent.name or licence.stem}.txt").write_bytes(licence.read_bytes())
    print(f"shinki_fonts: {len(text)} Japanese characters, fonts in {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
