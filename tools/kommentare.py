#!/usr/bin/env python3
"""Liest die technischen Kommentare zu Shinki Toho aus den PDFs von shinkiryu.de.

Aufruf aus dem Projektordner:  python3 tools/kommentare.py

Eingabe (quellen/shinki-toho/):
  Odachi Kommentare.pdf                        Kiriotoshi, Kumidachi-Einleitung, Ô-dachi 1–43
  shinkiryu.de/Kamae Kommentare.pdf  Kamae 1–24
  shinkiryu.de/Suburi.pdf            drei Arten Suburi

Ausgabe: daten/shinki-toho-kommentare.yaml (erzeugt, nicht von Hand bearbeiten). tools/daten.py
hängt die Texte an Kamae, Suburi, Kiriotoshi und Ô-dachi in mitori-daten.json. Braucht pdftotext.
"""
import re
import subprocess
from pathlib import Path

import yaml

WURZEL = Path(__file__).resolve().parent.parent
ST = WURZEL / "quellen" / "shinki-toho"
QUELLE = "https://www.shinkiryu.de/shinkitoho/de/"

# Kamae-Kommentar Nr. → Indizes in shinki-toho.yaml/kamae (Battô Seigan und Suriage-Men sind dort je zwei Einträge)
KAMAE = {1: [0], 2: [1, 2], 3: [3], 4: [4], 5: [5], 6: [6], 7: [7], 8: [8], 9: [9], 10: [10], 11: [11], 12: [12],
         13: [13], 14: [14], 15: [15], 16: [16], 17: [17], 18: [18], 19: [19], 20: [20, 21], 21: [22], 22: [23],
         23: [24], 24: [25]}


def text(pdf):
    return subprocess.run(["pdftotext", str(pdf), "-"], check=True, capture_output=True, text=True).stdout


def absaetze(zeilen):
    """Zeilen zu Absätzen; Seitenumbrüche mitten im Satz werden wieder zusammengefügt"""
    out = []
    leer = False  # Leerzeile seit der letzten Textzeile
    kurz = False  # letzte Zeile endete deutlich vor dem rechten Rand: Absatzende
    for z in zeilen:
        z = z.replace("\f", "").strip()
        if re.fullmatch(r"\d+", z):  # Seitenzahl
            continue
        if not z:
            leer = True
            continue
        satzende = not out or re.search(r'[.!?:)"“”]$', out[-1])
        if not out or z.startswith("*") or kurz or (leer and satzende):
            out.append(z)
        else:
            out[-1] = verbinde(out[-1], z)
        leer = False
        kurz = len(z) < 70
    # pdftotext entfernt Trennstriche am Zeilenende, auch in Komposita wie „Halbkreis-Bewegung“
    return re.sub(r"(?<=[a-zäöüß])(?=[A-ZÄÖÜ][a-zäöü])", "-", "\n\n".join(out))


def verbinde(a, b):
    return a + b if a.endswith("-") and not a.endswith(" -") else a + " " + b


def odachi(t):
    zeilen = t.splitlines()
    start = next(i for i, z in enumerate(zeilen) if z.startswith("Grundübung: Kiriotoshi"))
    ende = next(i for i, z in enumerate(zeilen) if "Kumidachi 組太刀" in z)
    kiri = absaetze(zeilen[start + 2:ende])
    erste = next(i for i, z in enumerate(zeilen) if re.match(r"1\. Ichi-ban/", z))
    formen, cur = [], None
    for z in zeilen[erste:]:
        m = re.match(r"(\d+)\. (.+?-?ban.*)$", z)
        if m and int(m.group(1)) == len(formen) + 1:
            cur = {"nr": int(m.group(1)), "titel": m.group(2).strip(), "zeilen": []}
            formen.append(cur)
        elif cur:
            cur["zeilen"].append(z)
    return kiri, einleitung(zeilen[ende:erste]), [{"nr": f["nr"], "titel": f["titel"], "text": absaetze(f["zeilen"])} for f in formen]


def einleitung(zeilen):
    """Kumidachi-Einleitung ohne die Übersichtstabelle; Zwischenüberschriften als „## …“"""
    tab = next(i for i, z in enumerate(zeilen) if z.startswith("Übersicht der Ô-dachi"))
    rei = next(i for i, z in enumerate(zeilen) if z.startswith("Rei-ho"))
    schluss = next(i for i, z in enumerate(zeilen) if z.startswith("Itto-den Shinki Toho, Shoden"))
    vorn = zeilen[2:tab]
    liste = next(i for i, z in enumerate(vorn) if z.startswith("Kumidachi im Shinki Toho"))
    paare = [z.strip() for z in vorn[liste + 1:] if z.strip()]
    waffen = "\n".join(f"{a} {b}" for a, b in zip(paare[:len(paare) // 2], paare[len(paare) // 2:]))
    teile = [absaetze(vorn[:liste + 1]), waffen]
    block = []
    for z in zeilen[rei:schluss] + ["Ende"]:
        if re.match(r"(Rei-ho |Anfang des Kumidachi|Ende$)", z):
            if block:
                teile.append(absaetze(block))
            if z != "Ende":
                teile.append("## " + z.strip())
            block = []
        else:
            block.append(z)
    return "\n\n".join(teile)


def kamae(t):
    zeilen = t.splitlines()
    eintraege, cur, i = [], None, 0
    while i < len(zeilen):
        z = zeilen[i].strip()
        m = re.fullmatch(r"(\d+)\.\s*(.*)", z)
        if m and int(m.group(1)) == len(eintraege) + 1:
            titel = m.group(2)
            while not titel:  # „1.“ und Name stehen in pdftotext getrennt
                i += 1
                titel = zeilen[i].strip()
            cur = {"nr": int(m.group(1)), "titel": titel, "zeilen": []}
            eintraege.append(cur)
        elif cur:
            cur["zeilen"].append(zeilen[i])
        i += 1
    return [{"nr": e["nr"], "titel": e["titel"], "kamae": KAMAE[e["nr"]], "text": absaetze(e["zeilen"])}
            for e in eintraege]


def suburi(t):
    zeilen = t.splitlines()
    arten, cur = [], None
    for z in zeilen:
        if re.match(r'^(Shomen-Uchioroshi|Shomen-Uchikomi|Te no Uchi) \("', z):
            cur = {"titel": z.strip(), "zeilen": []}
            arten.append(cur)
        elif cur:
            cur["zeilen"].append(z)
    return [{"nr": n, "titel": a["titel"], "text": absaetze(a["zeilen"])} for n, a in enumerate(arten, 1)]


def main():
    kiri, kumidachi, formen = odachi(text(ST / "Odachi Kommentare.pdf"))
    k = kamae(text(ST / "shinkiryu.de" / "Kamae Kommentare.pdf"))
    s = suburi(text(ST / "shinkiryu.de" / "Suburi.pdf"))
    assert len(formen) == 43, len(formen)
    assert len(k) == 24, len(k)
    assert len(s) == 3, len(s)
    daten = {
        "quelle": {"url": QUELLE, "titel": "Itto-den Shinki Toho, Beschreibung der Formen (shinkiryu.de)",
                   "dateien": ["Odachi Kommentare.pdf", "Kamae Kommentare.pdf", "Suburi.pdf"]},
        "kiriotoshi": kiri, "kumidachi": kumidachi, "suburi": s, "kamae": k, "odachi": formen,
    }
    ziel = WURZEL / "daten" / "shinki-toho-kommentare.yaml"
    ziel.write_text("# erzeugt von tools/kommentare.py aus den PDFs von shinkiryu.de, nicht von Hand bearbeiten\n"
                    + yaml.safe_dump(daten, allow_unicode=True, sort_keys=False, width=110), encoding="utf-8")
    print(f"kommentare: {len(formen)} Ô-dachi, {len(k)} Kamae, {len(s)} Suburi → {ziel.relative_to(WURZEL)}")


if __name__ == "__main__":
    main()
