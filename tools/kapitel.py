#!/usr/bin/env python3
"""Ordnet die Kapitel der Prüfungsvideos den Techniken zu und schreibt daten/videos.yaml.

Aufruf aus dem Projektordner:  python3 tools/kapitel.py [--videos "../Mitori Videos"] [--nur aikido/k4]

Je Kapitel liest es mit tesseract (deu) das erste Bild nach dem Anfang, auf dem ein Titel steht
(1,5 / 3 / 5 / 8 s): den Balken oben (Gruppe oder Angriff), den Balken unten (Technik) und bei
schwarzem Bild die Titeltafel in der Mitte. Dann richtet es die erkannten Texte an der Reihenfolge
der Techniken des Grades aus (Needleman-Wunsch): Kapitel ohne passende Technik (Vorspann,
Zwischentitel) fallen weg, eine Technik ohne Kapitel bleibt ohne Videostelle. Eine Lücke zwischen
zwei zugeordneten Nachbarn mit gleich vielen Kapiteln wie Techniken füllt es der Reihe nach
(Kapitel ohne Titel, z. B. Jiyu-waza). Das Kapitel endet am Anfang des nächsten.

daten/videos.yaml ist erzeugt, aber nicht in der CI prüfbar (die Videos liegen nicht im Repo):
nach jedem Lauf die Einträge mit `pruefen: true` von Hand gegen das Video ansehen.
Braucht ffmpeg/ffprobe, tesseract mit deu und PyYAML. Erkannte Texte werden zwischengespeichert
(MITORI_KAPITEL_CACHE, Standard ~/.cache/claude/mitori-kapitel).
"""
import argparse
import difflib
import json
import os
import re
import subprocess
import sys
import unicodedata
from pathlib import Path

import yaml

WURZEL = Path(__file__).resolve().parent.parent
CACHE = Path(os.environ.get("MITORI_KAPITEL_CACHE", Path.home() / ".cache" / "claude" / "mitori-kapitel"))
BILDER_NACH = (1.5, 3.0, 5.0, 8.0)  # Sekunden nach dem Kapitelanfang: das erste Bild mit Titel zählt
LUECKE = -0.1         # Wert je übersprungenem Kapitel oder Technik in der Ausrichtung
SCHWELLE = 0.35       # Ähnlichkeit, ab der eine Zuordnung mehr bringt als zwei Lücken
SICHER = 0.6          # ab dieser Ähnlichkeit gilt eine Zuordnung als sicher

# Datei im Videoordner → (Disziplin, Grad). Shinki Toho: eigene Liste (siehe stoff_shinki_toho).
VIDEOS = {
    **{f"Aikido/{n}. Kyu.mkv": ("aikido", f"k{n}") for n in range(1, 6)},
    **{f"Aikido/{n}. Dan.mp4": ("aikido", f"d{n}") for n in range(1, 4)},
    **{f"Daitoryu/{n}. Kyu.mkv": ("daitoryu", f"k{n}") for n in range(1, 6)},
    **{f"Daitoryu/{n}. Dan.mkv": ("daitoryu", f"d{n}") for n in range(1, 4)},
    "Daitoryu/3. Dan Okugi.mkv": ("daitoryu", "ok"),
    "Shinki Toho/Shoden.mkv": ("shinkitoho", "shoden"),
    "Shinki Toho/Chuden.mkv": ("shinkitoho", "chuden"),
    "Shinki Toho/Suburi.mkv": ("shinkitoho", "suburi"),
}


# Bildbereich ohne die schwarzen Balken mit den eingebrannten Titeln (app-entscheidungen.md, Player):
# y und Höhe in Pixeln der Quelle, dazu das Seitenverhältnis des sichtbaren Bildes (Pixel 16:15).
BESCHNITT = {
    "aikido": {"hoehe": 478, "y": 64, "h": 372, "verhaeltnis": round(768 / 372, 4)},
    "daitoryu": {"hoehe": 464, "y": 50, "h": 374, "verhaeltnis": round(768 / 374, 4)},
    "shinkitoho": {"hoehe": 576, "y": 0, "h": 576, "verhaeltnis": round(16 / 9, 4)},
}


def kapitel(datei):
    aus = subprocess.run(["ffprobe", "-v", "error", "-show_chapters", "-show_entries", "format=duration",
                          "-of", "json", str(datei)], check=True, capture_output=True, text=True).stdout
    daten = json.loads(aus)
    ende = float(daten["format"]["duration"])
    starts = [float(k["start_time"]) for k in daten["chapters"]]
    return [(s, (starts[i + 1] if i + 1 < len(starts) else ende)) for i, s in enumerate(starts)]


def ocr(bild, psm):
    return subprocess.run(["tesseract", str(bild), "-", "-l", "deu", "--psm", str(psm)],
                          check=True, capture_output=True, text=True).stdout.strip()


def lies_bild(datei, zeit):
    """Texte (oben, unten, mitte, tafel) des Bildes bei `zeit` Sekunden, zwischengespeichert."""
    schluessel = CACHE / f"{re.sub(r'[^a-z0-9]+', '-', str(datei).lower())}-t{zeit:.2f}.json"
    if schluessel.exists():
        return json.loads(schluessel.read_text(encoding="utf-8"))
    CACHE.mkdir(parents=True, exist_ok=True)
    bild = CACHE / f"bild-{os.getpid()}.png"
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-ss", f"{zeit:.2f}", "-i", str(datei),
                    "-frames:v", "1", "-vf", "scale=1440:-1,format=gray", str(bild)], check=True)
    texte = {}
    for name, crop in (("oben", "iw:ih*0.14:0:0"), ("unten", "iw:ih*0.13:0:ih*0.87"), ("mitte", "iw:ih*0.5:0:ih*0.25")):
        teil = CACHE / f"teil-{name}-{os.getpid()}.png"
        # Weiße Schrift auf Schwarz: invertiert liest tesseract besser.
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", str(bild), "-vf", f"crop={crop},negate", str(teil)], check=True)
        texte[name] = " ".join(ocr(teil, 6).split())
    schwarz = subprocess.run(["magick", str(bild), "-format", "%[fx:mean]", "info:"],
                             check=True, capture_output=True, text=True).stdout
    texte["tafel"] = float(schwarz) < 0.08
    for f in CACHE.glob(f"*-{os.getpid()}.png"):
        f.unlink()
    schluessel.write_text(json.dumps(texte, ensure_ascii=False), encoding="utf-8")
    return texte


def hat_titel(texte):
    lesbar = lambda t: len(re.findall(r"[A-Za-z]{3,}", t)) > 0
    return lesbar(texte["mitte"]) if texte["tafel"] else lesbar(texte["unten"])


def lies(datei, start, ende):
    """Texte des ersten Bildes im Kapitel, auf dem ein Titel steht (sonst des ersten)."""
    erstes = None
    for nach in BILDER_NACH:
        if start + nach >= ende:
            break
        texte = lies_bild(datei, start + nach)
        erstes = erstes or texte
        if hat_titel(texte):
            return texte
    return erstes or {"oben": "", "unten": "", "mitte": "", "tafel": False}


def norm(text):
    text = unicodedata.normalize("NFKD", text or "").encode("ascii", "ignore").decode().lower()
    return re.sub(r"[^a-z0-9]+", "", text)


def aehnlich(a, b):
    a, b = norm(a), norm(b)
    if not a or not b:
        return 0.0
    return difflib.SequenceMatcher(None, a, b).ratio()


def wertung(texte, eintrag):
    """Ähnlichkeit 0..1 zwischen den Texten eines Kapitels und einem Eintrag des Stoffs."""
    name, angriff = eintrag["name"], eintrag.get("angriff") or ""
    if texte["tafel"]:
        return aehnlich(texte["mitte"], f"{angriff} {name}") if angriff else max(
            aehnlich(texte["mitte"], name), aehnlich(texte["mitte"][:len(name) + 6], name))
    unten = aehnlich(texte["unten"], name)
    if angriff:
        return 0.6 * unten + 0.4 * aehnlich(texte["oben"], angriff)
    return unten


def ausrichten(texte, stoff):
    """Needleman-Wunsch: jedes Kapitel höchstens eine Technik, Reihenfolge bleibt."""
    n, m = len(texte), len(stoff)
    w = [[wertung(texte[i], stoff[j]) for j in range(m)] for i in range(n)]
    # Ein schwacher Treffer zählt negativ: ein Kapitel ohne Titel gewinnt nicht gegen zwei Lücken.
    g = [[w[i][j] - SCHWELLE for j in range(m)] for i in range(n)]
    best = [[0.0] * (m + 1) for _ in range(n + 1)]
    weg = [[None] * (m + 1) for _ in range(n + 1)]
    for i in range(1, n + 1):
        best[i][0], weg[i][0] = best[i - 1][0] + LUECKE, "k"
    for j in range(1, m + 1):
        best[0][j], weg[0][j] = best[0][j - 1] + LUECKE, "t"
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            best[i][j], weg[i][j] = max((best[i - 1][j - 1] + g[i - 1][j - 1], "p"),
                                        (best[i - 1][j] + LUECKE, "k"), (best[i][j - 1] + LUECKE, "t"))
    paare, i, j = {}, n, m
    while i or j:
        schritt = weg[i][j]
        if schritt == "p":
            paare[j - 1] = (i - 1, w[i - 1][j - 1])
            i, j = i - 1, j - 1
        elif schritt == "k":
            i -= 1
        else:
            j -= 1
    return luecken_fuellen(paare, texte, m)


def luecken_fuellen(paare, texte, m):
    """Zwischen zwei Nachbarn: gleich viele freie Kapitel ohne Tafel wie Techniken → der Reihe nach."""
    belegt = sorted((j, i) for j, (i, _) in paare.items())
    grenzen = [(-1, -1)] + belegt + [(m, len(texte))]
    for (j1, i1), (j2, i2) in zip(grenzen, grenzen[1:]):
        freie_t = list(range(j1 + 1, j2))
        freie_k = [i for i in range(i1 + 1, i2) if not texte[i]["tafel"]]
        if freie_t and len(freie_t) == len(freie_k) and -1 < j1 and j2 < m:
            for j, i in zip(freie_t, freie_k):
                paare[j] = (i, 0.0)
    return paare


def stoff_shinki_toho(st, video):
    """Was die Shinki-Toho-Videos zeigen: Shoden Ô-dachi 1–11, Chuden 12–21, Suburi die drei
    Suburi und die Kamae-Übung (ein Kapitel, die Übung ist ein Ablauf). Kiriotoshi zeigt keins."""
    if video == "suburi":
        kamae = st["kamae"]
        return ([{"id": s["id"], "name": s["name"]} for s in st["suburi"]]
                + [{"id": kamae["id"], "name": kamae["name"].split("-")[0]}])
    bis = next(g["bis"] for g in st["grade"] if g["id"] == video)
    von = 1 if video == "shoden" else next(g["bis"] for g in st["grade"] if g["id"] == "shoden") + 1
    return [{"id": o["id"], "name": f"{o['nr']}. {o['name'].replace(' · ', ' ')}"} for o in st["odachi"] if von <= o["nr"] <= bis]


def stoff(daten, disziplin, grad):
    d = next(x for x in daten["disziplinen"] if x["id"] == disziplin)
    if disziplin == "shinkitoho":
        return stoff_shinki_toho(d, grad)
    text = {g["id"] for g in d["gruppen"] if g["art"] == "text"}  # Theorie: Aufsatz, mündliche Prüfung
    return [{"id": t["id"], "name": t["name"], "angriff": (t.get("angriff") or {}).get("name")}
            for t in d["techniken"] if t["grad"] == grad and t["gruppe"] not in text]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--videos", default=str(WURZEL.parent / "Mitori Videos"))
    ap.add_argument("--nur", help="nur dieses Video, z. B. aikido/k4")
    ap.add_argument("--zeigen", action="store_true", help="erkannte Texte und Zuordnung ausgeben, nichts schreiben")
    args = ap.parse_args()
    subprocess.run([sys.executable, str(WURZEL / "tools" / "daten.py")], check=True, stdout=subprocess.DEVNULL)
    daten = json.loads((WURZEL / "build" / "daten" / "mitori-daten.json").read_text(encoding="utf-8"))

    ausgabe = {}
    for datei, (disziplin, grad) in VIDEOS.items():
        schluessel = f"{disziplin}/{grad}"
        if args.nur and args.nur != schluessel:
            continue
        pfad = Path(args.videos) / datei
        kap = kapitel(pfad)
        texte = [lies(pfad, von, bis) for von, bis in kap]
        liste = stoff(daten, disziplin, grad)
        paare = ausrichten(texte, liste)
        eintraege = []
        for j, e in enumerate(liste):
            if j not in paare:
                eintraege.append({"id": e["id"], "fehlt": True})
                continue
            i, w = paare[j]
            bis = kap[i][1]
            if disziplin == "shinkitoho":
                # Eine Form geht über ihre Titeltafel und die folgenden Kapitel ohne Tafel (zweite Ausführung).
                naechste = next((k for k in range(i + 1, len(kap)) if texte[k]["tafel"]), len(kap))
                bis = kap[naechste - 1][1]
            eintrag = {"id": e["id"], "von": round(kap[i][0], 2), "bis": round(bis, 2)}
            if w < SICHER:
                eintrag["pruefen"] = True
            eintraege.append(eintrag)
            if args.zeigen:
                t = texte[i]
                erkannt = t["mitte"] if t["tafel"] else f"{t['oben']} | {t['unten']}"
                print(f"{'?' if w < SICHER else ' '} {i + 1:3} {w:.2f}  {erkannt[:50]:50}  {e.get('angriff') or ''} {e['name']}")
        if args.zeigen:
            frei = sorted(set(range(len(kap))) - {i for i, _ in paare.values()})
            print(f"{schluessel}: {len(kap)} Kapitel, {len(liste)} Einträge, {len(paare)} zugeordnet, "
                  f"ohne Technik: {[i + 1 for i in frei]}\n")
        ausgabe[schluessel] = {"datei": datei, "beschnitt": BESCHNITT[disziplin], "kapitel": eintraege}

    if args.zeigen:
        return
    ziel = WURZEL / "daten" / "videos.yaml"
    alt = yaml.safe_load(ziel.read_text(encoding="utf-8")) if ziel.exists() else {}
    alt.update(ausgabe)
    kopf = ("# Videostellen je Technik: erzeugt von tools/kapitel.py aus den Kapiteln der Prüfungsvideos.\n"
            "# Einträge mit `pruefen: true` von Hand gegen das Video prüfen und das Feld dann löschen;\n"
            "# von Hand geänderte Zeiten überschreibt ein neuer Lauf (darum nur mit --nur neu erzeugen).\n")
    ziel.write_text(kopf + yaml.safe_dump(alt, allow_unicode=True, sort_keys=False, width=120), encoding="utf-8")
    print(f"kapitel: {sum(len(v['kapitel']) for v in ausgabe.values())} Einträge → {ziel}")


if __name__ == "__main__":
    main()
