"""Shinki (shinki/README.md): every text reaches WCAG AA (4.5:1) on each ground the bible allows
it on (section 2), and ShinkiStyle.qml is the one generated from tokens.json (tools/shinki_style.py),
so no value lives twice."""
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOKENS = json.loads((ROOT / "shinki" / "tokens.json").read_text(encoding="utf-8"))
AA = 4.5

# Text role → the grounds it may stand on (section 2 and 5 of the bible).
PAIRS = {
    "ink": ["paper", "paperAlt", "chip", "selected"],
    "inkMuted": ["paper", "paperAlt", "chip", "selected"],
    "red": ["paper", "paperAlt", "chip"],
    "onRed": ["red"],
    "videoText": ["video"],
    "videoMuted": ["video"],
}


def luminance(hex_colour):
    """Relative luminance (WCAG 2.x) of #RRGGBB."""
    channels = [int(hex_colour[i:i + 2], 16) / 255 for i in (1, 3, 5)]
    linear = [c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in channels]
    return 0.2126 * linear[0] + 0.7152 * linear[1] + 0.0722 * linear[2]


def contrast(a, b):
    la, lb = sorted((luminance(a), luminance(b)), reverse=True)
    return (la + 0.05) / (lb + 0.05)


def test_contrast_luminance_reference():
    # Black on white is 21:1 (the formula, not the tokens).
    assert round(contrast("#000000", "#FFFFFF"), 1) == 21.0


def test_text_reaches_aa():
    colour = TOKENS["color"]
    weak = [f"{text} on {ground}: {contrast(colour[text], colour[ground]):.2f}"
            for text, grounds in PAIRS.items() for ground in grounds
            if contrast(colour[text], colour[ground]) < AA]
    assert not weak, weak


def test_level_numbers_readable():
    # A level's number stands on its colour in the role `levelText` names (ink, on level 4 onRed).
    colour, levels, texts = TOKENS["color"], TOKENS["level"], TOKENS["levelText"]
    assert len(texts) == len(levels)
    for i, ground in enumerate(levels):
        assert contrast(colour[texts[i]], ground) >= AA, (i, contrast(colour[texts[i]], ground))


def test_style_is_generated():
    result = subprocess.run([sys.executable, str(ROOT / "tools" / "shinki_style.py"), "--check"],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr


FONTS = ROOT / "shinki" / "fonts"


def cmap(path):
    """The code points of a TrueType font (cmap formats 4 and 12), without fontTools."""
    import struct
    data = path.read_bytes()
    count = struct.unpack(">H", data[4:6])[0]
    tables = {data[12 + 16 * i:16 + 16 * i]: struct.unpack(">II", data[20 + 16 * i:28 + 16 * i])
              for i in range(count)}
    offset = tables[b"cmap"][0]
    points = set()
    for i in range(struct.unpack(">H", data[offset + 2:offset + 4])[0]):
        sub = offset + struct.unpack(">I", data[offset + 8 + 8 * i:offset + 12 + 8 * i])[0]
        fmt = struct.unpack(">H", data[sub:sub + 2])[0]
        if fmt == 12:
            groups = struct.unpack(">I", data[sub + 12:sub + 16])[0]
            for g in range(groups):
                start, end, _ = struct.unpack(">III", data[sub + 16 + 12 * g:sub + 28 + 12 * g])
                points |= set(range(start, end + 1))
        elif fmt == 4:
            segs = struct.unpack(">H", data[sub + 6:sub + 8])[0] // 2
            ends = struct.unpack(f">{segs}H", data[sub + 14:sub + 14 + 2 * segs])
            starts = struct.unpack(f">{segs}H", data[sub + 16 + 2 * segs:sub + 16 + 4 * segs])
            for s, e in zip(starts, ends):
                points |= set(range(s, e + 1)) - {0xFFFF}
    return points


def test_fonts_there():
    for name in ["PTSans-Regular.ttf", "PTSans-Bold.ttf", "PTSansNarrow-Bold.ttf", "CrimsonPro.ttf",
                 "NotoSerifJP-Regular-Subset.ttf"]:
        assert (FONTS / name).exists(), name
    assert list(FONTS.glob("OFL-*.txt")), "the licences go with the fonts"


def test_japanese_subset_covers_chars():
    # Every character of chars-full.txt is in the subset (else: tools/shinki_fonts.py).
    sys.path.insert(0, str(ROOT / "tools"))
    from shinki_fonts import japanese_chars
    missing = sorted(ch for ch in japanese_chars() if ord(ch) not in cmap(FONTS / "NotoSerifJP-Regular-Subset.ttf"))
    assert not missing, "".join(missing)


def test_chars_full_covers_app():
    # Every Japanese character the app shows is listed (else: tools/shinki_chars.py, then shinki_fonts.py).
    sys.path.insert(0, str(ROOT / "tools"))
    from shinki_chars import QUELLEN, collect
    assert all(q.is_dir() for q in QUELLEN), QUELLEN
    listed = (FONTS / "chars-full.txt").read_text(encoding="utf-8")
    missing = sorted(set(collect(QUELLEN)) - set(listed))
    assert not missing, "".join(missing)


def test_chars_skips_binary(tmp_path):
    # A PDF or a font read as text yields thousands of false kanji.
    sys.path.insert(0, str(ROOT / "tools"))
    from shinki_chars import collect
    (tmp_path / "a.qml").write_text("見", encoding="utf-8")
    (tmp_path / "b.pdf").write_text("取", encoding="utf-8")
    assert collect([tmp_path]) == "見"


def test_latin_covers_german():
    points = cmap(FONTS / "CrimsonPro.ttf") & cmap(FONTS / "PTSans-Regular.ttf")
    # The fonts have no arrows (→); Qt takes them from a fallback font.
    assert all(ord(ch) in points for ch in "AÄÖÜäöüßéèô€„“–—…·"), "German and the app's signs"


def family(path):
    """The font's family (name ID 16, else 1; Windows Unicode records), without fontTools."""
    import struct
    data = path.read_bytes()
    count = struct.unpack(">H", data[4:6])[0]
    tables = {data[12 + 16 * i:16 + 16 * i]: struct.unpack(">II", data[20 + 16 * i:28 + 16 * i])
              for i in range(count)}
    offset = tables[b"name"][0]
    records, strings = struct.unpack(">HH", data[offset + 2:offset + 6])
    names = {}
    for i in range(records):
        platform, _, _, name_id, length, start = struct.unpack(">6H", data[offset + 6 + 12 * i:offset + 18 + 12 * i])
        if platform == 3:
            names[name_id] = data[offset + strings + start:offset + strings + start + length].decode("utf-16-be")
    return names.get(16, names.get(1))


def test_font_families_match_tokens():
    # Qt finds a font by its family: each file names one of the bible's families.
    families = set(TOKENS["font"].values())
    wrong = {p.name: family(p) for p in FONTS.glob("*.ttf") if family(p) not in families}
    assert not wrong, wrong


def test_reserved_names_unmodified():
    # OFL: a font with a Reserved Font Name ships unmodified (no subset): PT Sans (ParaType).
    # The whole PT Sans has Cyrillic; a Latin subset would not.
    for name in ["PTSans-Regular.ttf", "PTSans-Bold.ttf", "PTSansNarrow-Bold.ttf"]:
        assert 0x0416 in cmap(FONTS / name), f"{name} is a subset (OFL: Reserved Font Name)"
