#!/usr/bin/env python3
"""Liest das Aiki-Budo-Lexikon von shinkiryu.de (Japanisch–Deutsch) für das Glossar der App.

Aufruf aus dem Projektordner:  python3 tools/lexikon.py

Eingabe:  quellen/shinki-toho/shinkiryu.de/Aiki-Budo-Lexikon.pdf (Stand 20. März 2018)
Ausgabe:  daten/lexikon.yaml (erzeugt, nicht von Hand bearbeiten): je Wort Kanji (falls angegeben) und
          Übersetzung. tools/daten.py übernimmt die Liste nach mitori-daten.json. Braucht pdftotext.
"""
import re
import subprocess
from pathlib import Path

import yaml

WURZEL = Path(__file__).resolve().parent.parent
PDF = WURZEL / "quellen" / "shinki-toho" / "shinkiryu.de" / "Aiki-Budo-Lexikon.pdf"
CJK = r"[　-〿぀-ヿ一-鿿、]"


def main():
    text = subprocess.run(["pdftotext", "-layout", str(PDF), "-"], check=True, capture_output=True, text=True).stdout
    zeilen = text.split("Wörter (alphabetisch)", 1)[1].split("japanische Zahlen:", 1)[0].splitlines()  # Zahlen gehören nicht ins Glossar
    woerter = []
    for z in zeilen:
        z = z.replace("\f", "").rstrip()
        if not z.strip() or re.fullmatch(r"\s*\d+\s*", z):
            continue
        if z[0].isspace() or z.startswith("…"):  # Fortsetzung der Übersetzung
            if woerter:
                woerter[-1]["de"] = (woerter[-1]["de"] + " " + z.strip()).strip()
            continue
        m = re.match(rf"(.+?)(?:\s+({CJK}+(?:\s*{CJK}+)*)|\s{{2,}}|$)(.*)", z)
        wort, jp, de = m.group(1).strip(), (m.group(2) or "").replace(" ", ""), re.sub(r"\s{2,}", " ", m.group(3)).strip()
        woerter.append({"wort": wort, "jp": jp or None, "de": de})
    for w in woerter:
        w["de"] = w["de"].replace("( ", "(")
    ohne = [w["wort"] for w in woerter if not w["de"]]  # Zeilen, deren Spalten pdftotext nicht trennt
    woerter = [w for w in woerter if w["de"]]
    ziel = WURZEL / "daten" / "lexikon.yaml"
    ziel.write_text("# erzeugt von tools/lexikon.py aus dem Aiki-Budo-Lexikon (shinkiryu.de, Stand 20.03.2018), nicht von Hand bearbeiten\n"
                    + yaml.safe_dump({"quelle": "Lexikon der Aiki-Budo-Begriffe, Japanisch–Deutsch, shinkiryu.de, Stand 20. März 2018",
                                      "woerter": woerter}, allow_unicode=True, sort_keys=False, width=110), encoding="utf-8")
    print(f"lexikon: {len(woerter)} Wörter, {sum(1 for w in woerter if w['jp'])} mit Kanji → {ziel.relative_to(WURZEL)}"
          + (f"; übersprungen: {ohne}" if ohne else ""))


if __name__ == "__main__":
    main()
