#!/usr/bin/env python3
"""Makes Shinki's fonts (shinki/README.md, section 3) for the app: shinki/fonts/*.ttf.

  tools/shinki_fonts.py <folder with the fonts of google/fonts (ofl/…)>

  PT Sans Regular and Bold, PT Sans Narrow Bold
      unchanged: their OFL reserves the name "PT Sans", a subset (a Modified Version) may not keep it.
  Crimson Pro (variable, 200–900)
      Latin only (the app's languages are German and English): a third of the size.
  Noto Serif JP Regular (an instance of the variable font)
      only the kanji and kana of shinki/fonts/chars-full.txt (tools/shinki_chars.py).
      The whole font has 13 MB; tests/test_shinki.py checks that the subset still covers
      every character, so a new kanji in the data fails until this tool runs again.

Needs fontTools (pyftsubset); the CI does not, it reads the committed subsets.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "shinki" / "fonts"
CHARS = OUT / "chars-full.txt"

# Latin, Latin-1, Latin Extended-A/B, general punctuation and €. Crimson Pro has no arrows (→);
# Qt takes them from a fallback font.
LATIN = "U+0000-024F,U+02C6-02DC,U+2000-206F,U+20AC,U+2212,U+2264-2265,U+25A0-25FF"
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


def japanese_chars():
    """Every Japanese character listed in shinki/fonts/chars-full.txt."""
    if not CHARS.exists():
        sys.exit(f"shinki_fonts: {CHARS.relative_to(ROOT)} fehlt (tools/shinki_chars.py)")
    text = CHARS.read_text(encoding="utf-8")
    return {ch for ch in text if ch.strip()}


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
