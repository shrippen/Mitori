#!/usr/bin/env python3
"""Schreibt die japanischen Zeichen der App nach shinki/fonts/chars-full.txt, für tools/shinki_fonts.py.

  python3 tools/shinki_chars.py
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
# Was die App zeigt: ihre Texte, Shinkis QML, die Programmdaten.
QUELLEN = [ROOT / "app", ROOT / "shinki" / "qml", ROOT / "third_party" / "mitori-daten" / "daten"]
LISTE = ROOT / "shinki" / "fonts" / "chars-full.txt"

# Nur Quelltext: ein PDF oder eine Schrift als Text gelesen ergibt Tausende falscher Kanji.
TEXT = {".yaml", ".qml", ".js", ".ts", ".cpp", ".h"}

# Kana, Kana-Erweiterung, Kanji (CJK), CJK-Kompatibilität und voller
# Halbbreiten-Block. Werkzeug des shinki-Stils; shinki_fonts.py prüft dagegen.
JAPANESE_RANGES = [
    (0x3000, 0x30FF),   # Kana und Kana-Satzzeichen
    (0x3400, 0x4DBF),   # CJK-Erweiterung A
    (0x4E00, 0x9FFF),   # CJK-Ideogramme (BMP)
    (0xF900, 0xFAFF),   # CJK-Kompatibilität
    (0xFF00, 0xFFEF),   # Halbbreite Formen
]


def is_japanese(c):
    """Wahr, wenn c in einem der japanischen Unicode-Bereiche liegt."""
    o = ord(c)
    return any(lo <= o <= hi for lo, hi in JAPANESE_RANGES)


def collect(paths):
    chars = set()
    for root in paths:
        for datei in root.rglob("*"):
            # Build-Ordner (app/build-android) tragen Kopien und fremde Quellen.
            if datei.suffix not in TEXT or not datei.is_file() \
                    or any(t.startswith("build") for t in datei.relative_to(root).parts[:-1]):
                continue
            chars |= {c for c in datei.read_text(encoding="utf-8", errors="ignore") if is_japanese(c)}
    return "".join(sorted(chars))


if __name__ == "__main__":
    # Ein Zeilenende, damit git die Datei als Text sieht.
    LISTE.write_text(collect(QUELLEN) + "\n", encoding="utf-8")
    print(f"{LISTE.relative_to(ROOT)}: {LISTE.stat().st_size} Bytes")
