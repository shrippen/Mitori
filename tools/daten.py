#!/usr/bin/env python3
"""Prüft die Programmdaten in daten/ und erzeugt daraus Heft- und App-Daten.

Aufruf aus dem Projektordner:  python3 tools/daten.py [--ausgabe build/daten]

Eingabe (daten/):
  aikido.yaml, daitoryu.yaml   Prüfungsprogramme (Aufbau im Kopf von aikido.yaml)
  shinki-toho.yaml             Grade, Kamae, Suburi, Ô-dachi-Formen
  shinki-toho-kommentare.yaml  Kommentare von shinkiryu.de (erzeugt von tools/kommentare.py)
  lexikon.yaml                 Aiki-Budo-Lexikon Japanisch–Deutsch (erzeugt von tools/lexikon.py)
  videos.yaml                  Videostelle je Technik (erzeugt von tools/kapitel.py, von Hand geprüft)
  begriffe.yaml                Kanji (Aikido, Shinki Toho), gesichert
  daitoryu-begriffe.yaml       Kanji-Bausteine Daitoryu, ungeprüft
  ../KORREKTUREN.md            offene Fragen; jede Frage-ID der Daten muss dort stehen

Ausgabe (build/daten/):
  <disziplin>.lua              für das Heft (heft/lua/mitori-heft.lua), Kanji schon eingesetzt
  mitori-daten.json            für die App: jede Technik einzeln mit fester ID, Videostellen,
                               je Video Datei (relativ zum Videoordner) und Beschnitt
  fehlende-kanji.txt           Namen, zu denen es kein Kanji gibt

Bricht mit Exit-Code 1 ab, wenn die Daten Fehler haben (unbekannter Grad, unbekannte
Frage-ID, unbekannter Schlüssel).
"""
import argparse
import json
import re
import sys
import unicodedata
from pathlib import Path

import yaml

WURZEL = Path(__file__).resolve().parent.parent
DATEN = WURZEL / "daten"
FORMAT = 1  # Version des App-Formats; erhöhen, wenn sich mitori-daten.json inkompatibel ändert

fehler: list[str] = []
fehlend: set[str] = set()


def melde(text):
    fehler.append(text)


# --- Namen und Kanji ---------------------------------------------------------

def schluessel(name):
    """klein, ohne Längenzeichen (ô → o)"""
    zerlegt = unicodedata.normalize("NFD", name)
    return "".join(z for z in zerlegt if unicodedata.category(z) != "Mn").lower()


def flach(name):
    """Aikido-Glossar: zusätzlich ohne Bindestriche und Leerzeichen"""
    return re.sub(r"[- ]", "", schluessel(name))


def kennung(text):
    """ID-Teil: klein, ASCII, Bindestriche"""
    return re.sub(r"[^a-z0-9]+", "-", schluessel(text).replace("'", "")).strip("-")


class Glossar:
    def __init__(self, datei, bausteine):
        roh = yaml.safe_load((DATEN / datei).read_text(encoding="utf-8"))
        self.bausteine = bausteine
        self.tabelle = {(schluessel(k) if bausteine else flach(k)): v for k, v in roh.items()}

    def _zusammensetzen(self, name):
        ganz = self.tabelle.get(schluessel(name))
        if ganz:
            return ganz
        teile = schluessel(name).split("-")
        erg, i = [], 0
        while i < len(teile):
            for j in range(len(teile), i, -1):
                k = "-".join(teile[i:j])
                if k in self.tabelle:
                    erg.append(self.tabelle[k])
                    i = j
                    break
            else:
                return None
        return "".join(erg)

    def kanji(self, art, name, leise=False):
        if not name:
            return None
        k = self._zusammensetzen(name) if self.bausteine else self.tabelle.get(flach(name))
        if k is None and not leise:
            fehlend.add(f"{art}: {name}")
        return k


def eintrag(text):
    """'Technik (Zusatz) [Frage Frage]' zerlegen"""
    fragen = []
    m = re.search(r"\s*\[(.*?)\]", text)
    if m:
        fragen = m.group(1).split()
        text = text[: m.start()] + text[m.end():]
    m = re.match(r"^(.*?)\s+\((.*)\)$", text)
    name, zusatz = (m.group(1), m.group(2)) if m else (text, None)
    return {"name": name.strip(), "zusatz": zusatz, "fragen": fragen}


# --- Korrekturliste ----------------------------------------------------------

def korrekturen():
    datei = WURZEL / "KORREKTUREN.md"
    ids = {}
    for zeile in datei.read_text(encoding="utf-8").splitlines():
        m = re.match(r"^\| ([ADS]-[A-Z0-9-]+) \|.*\| ([^|]+) \|$", zeile)
        if m:
            ids[m.group(1)] = m.group(2).strip()
    return ids


def pruefe_fragen(fragen, wo, ids):
    for f in fragen:
        if f not in ids:
            melde(f"{wo}: Frage {f} fehlt in KORREKTUREN.md")
        elif ids[f] != "klären":
            melde(f"{wo}: Frage {f} hat Status „{ids[f]}“; Marke aus den Daten nehmen oder Status ändern")


# --- Prüfungsprogramme (Aikido, Daitoryu) -----------------------------------

ANGRIFF_FELDER = {"angriff", "zusatz", "frage"}
GRUPPEN_FELDER = {"titel", "begriff", "ohne_angriff", "text", "frage", "angriffe", "fuss"}


def programm(art, ids, gloss):
    d = yaml.safe_load((DATEN / f"{art}.yaml").read_text(encoding="utf-8"))
    alle_grade = {g["id"] for g in d["grade"]}
    vorschlag = gloss.bausteine

    def jp_begriff(b):
        if not b:
            return None
        liste = [b] if isinstance(b, str) else b
        return "　".join(gloss.kanji(art, x) or "" for x in liste)

    heft = {k: d[k] for k in ("kurz", "titel", "untertitel", "jp", "stand", "vorbemerkung") if k in d}
    heft.update(vorschlag=vorschlag, grade=d["grade"], teile=[])
    app_gruppen, app_techniken = [], []

    for teil in d["teile"]:
        for gid in teil["grade"]:
            if gid not in alle_grade:
                melde(f"{art}, {teil['titel']}: unbekannter Grad {gid}")
        h_teil = {"titel": teil["titel"], "grade": teil["grade"], "gruppen": []}
        heft["teile"].append(h_teil)
        for gr in teil["gruppen"]:
            wo_g = f"{art}, {teil['titel']}, {gr['titel']}"
            for k in gr:
                if k not in GRUPPEN_FELDER:
                    melde(f"{wo_g}: unbekanntes Feld {k}")
            pruefe_fragen([gr["frage"]] if gr.get("frage") else [], wo_g, ids)
            ohne = bool(gr.get("ohne_angriff"))
            text = bool(gr.get("text"))
            g_id = f"{art}/{kennung(teil['titel'])}/{kennung(gr['titel'])}"
            h_gr = {"titel": gr["titel"], "jp": jp_begriff(gr.get("begriff")), "frage": gr.get("frage"),
                    "ohne_angriff": ohne, "text": text, "fuss": gr.get("fuss"), "angriffe": []}
            h_teil["gruppen"].append(h_gr)
            app_gruppen.append({"id": g_id, "teil": teil["titel"], "titel": gr["titel"], "jp": h_gr["jp"],
                                "art": "text" if text else ("ohne_angriff" if ohne else "angriffe")})
            for a in gr["angriffe"]:
                wo = f"{wo_g}, {a.get('angriff') or '–'}"
                for k in a:
                    if k not in ANGRIFF_FELDER and k not in teil["grade"]:
                        melde(f"{wo}: unbekanntes Feld oder Grad {k}")
                pruefe_fragen([a["frage"]] if a.get("frage") else [], wo, ids)
                name, zusatz = a.get("angriff"), a.get("zusatz")
                k1 = None if (ohne or text) else gloss.kanji(art, name)
                k2 = gloss.kanji(art, zusatz, leise=True) if (k1 and zusatz) else None
                a_jp = (k1 + "・" + k2) if (k1 and k2) else k1
                h_a = {"angriff": name, "zusatz": zusatz, "jp": a_jp, "frage": a.get("frage"), "techniken": {}}
                h_gr["angriffe"].append(h_a)
                a_id = kennung(" ".join(x for x in (name, zusatz) if x)) or "ohne"
                for gid in teil["grade"]:
                    liste = []
                    for nr, roh in enumerate(a.get(gid) or [], 1):
                        e = eintrag(roh)
                        pruefe_fragen(e["fragen"], f"{wo}, {gid}, {e['name']}", ids)
                        e["jp"] = None if text else gloss.kanji(art, e["name"])
                        liste.append(e)
                        app_techniken.append({
                            "id": f"{g_id}/{a_id}/{gid}/{kennung(e['name'])}-{nr}",
                            "gruppe": g_id, "grad": gid,
                            "angriff": None if ohne or text else {"name": name, "zusatz": zusatz, "jp": a_jp},
                            "name": e["name"], "zusatz": e["zusatz"], "jp": e["jp"],
                            "fragen": e["fragen"] + ([a["frage"]] if a.get("frage") else []),
                        })
                    if liste:
                        h_a["techniken"][gid] = liste
    app = {"id": art, "titel": d["titel"], "kurzname": d["kurzname"], "untertitel": d.get("untertitel"), "jp": d.get("jp"),
           "stand": d.get("stand"), "kanji_vorschlag": vorschlag, "grade": d["grade"],
           "teile": [{"titel": t["titel"], "grade": t["grade"]} for t in d["teile"]],
           "gruppen": app_gruppen, "techniken": app_techniken}
    return heft, app


# --- Shinki Toho -------------------------------------------------------------

def shinki_toho(ids):
    d = yaml.safe_load((DATEN / "shinki-toho.yaml").read_text(encoding="utf-8"))
    for g in d["grade"]:
        pruefe_fragen([g["frage"]] if g.get("frage") else [], f"shinki-toho, Grad {g['name']}", ids)
    nummern = [f["nr"] for f in d["odachi"]]
    if nummern != list(range(1, len(nummern) + 1)):
        melde("shinki-toho: Ô-dachi-Nummern nicht fortlaufend")
    for f in d["odachi"]:
        pruefe_fragen((f.get("frage") or "").split(), f"shinki-toho, Ô-dachi {f['nr']}", ids)
        for k in ("name", "uchidachi", "shidachi", "treffer", "zanshin"):
            if not f.get(k):
                melde(f"shinki-toho, Ô-dachi {f['nr']}: {k} fehlt")
    app = dict(d, id="shinkitoho")
    app["odachi"] = [dict(f, id=f"shinkitoho/odachi/{f['nr']}", fragen=(f.get("frage") or "").split())
                     for f in d["odachi"]]
    for f in app["odachi"]:
        f.pop("frage", None)
    # Die Kamae-Übung ist ein Ablauf: ein Eintrag, die Positionen sind seine Beschreibung.
    kamae = d["abschnitte"]["kamae"]
    app["kamae"] = {"id": "shinkitoho/kamae", "name": kamae["name"], "kanji": kamae["kanji"], "positionen": d["kamae"]}
    app["suburi"] = [dict(s, id=f"shinkitoho/suburi/{i}") for i, s in enumerate(d["suburi"], 1)]
    app["kiriotoshi"] = dict(d["kiriotoshi"], id="shinkitoho/kiriotoshi")

    # Kommentare von shinkiryu.de (tools/kommentare.py), nur für die App
    k = yaml.safe_load((DATEN / "shinki-toho-kommentare.yaml").read_text(encoding="utf-8"))
    app["kommentare_quelle"] = k["quelle"]
    app["kiriotoshi"]["kommentar"] = k["kiriotoshi"]
    kumi = d["abschnitte"]["kumidachi"]
    app["kumidachi"] = dict(kumi, id="shinkitoho/kumidachi",
                            kommentar={"titel": f"{kumi['name']} · {kumi['text'].split(':')[0]}", "text": k["kumidachi"]})
    for s, ks in zip(app["suburi"], k["suburi"], strict=True):
        s["kommentar"] = {"titel": ks["titel"], "text": ks["text"]}
    app["kamae"]["kommentar"] = {"titel": kamae["name"],
                                 "text": "\n\n".join(f"{kk['nr']}. {kk['titel']}\n{kk['text']}" for kk in k["kamae"])}
    for f, kf in zip(app["odachi"], k["odachi"], strict=True):
        f["kommentar"] = {"titel": f"{kf['nr']}. {kf['titel']}", "text": kf["text"]}
    return d, app


# --- Ausgabe -----------------------------------------------------------------

def lua(wert, tiefe=0):
    """Python-Wert als Lua-Ausdruck"""
    if wert is None:
        return "nil"
    if isinstance(wert, bool):
        return "true" if wert else "false"
    if isinstance(wert, (int, float)):
        return repr(wert)
    if isinstance(wert, str):
        return json.dumps(wert, ensure_ascii=False)  # JSON-Strings sind gültige Lua-Strings
    ein = "  " * (tiefe + 1)
    if isinstance(wert, list):
        return "{\n" + "".join(f"{ein}{lua(v, tiefe + 1)},\n" for v in wert) + "  " * tiefe + "}"
    teile = [f"{ein}[{json.dumps(k, ensure_ascii=False)}] = {lua(v, tiefe + 1)},\n"
             for k, v in wert.items() if v is not None]
    return "{\n" + "".join(teile) + "  " * tiefe + "}"


def videostellen(disziplinen):
    """Hängt an jeden Eintrag mit Kapitel in daten/videos.yaml seine Videostelle (video, von, bis)."""
    datei = DATEN / "videos.yaml"
    if not datei.exists():
        return 0, {}
    eintraege = {}
    for d in disziplinen:
        for e in d.get("techniken", []):
            eintraege[e["id"]] = e
        for art in ("suburi", "odachi"):
            for e in d.get(art, []):
                eintraege[e["id"]] = e
        for art in ("kamae", "kiriotoshi"):
            if art in d:
                eintraege[d[art]["id"]] = d[art]
    gesetzt = 0
    alle = yaml.safe_load(datei.read_text(encoding="utf-8")) or {}
    for video, inhalt in alle.items():
        for k in inhalt["kapitel"]:
            ziel = eintraege.get(k["id"])
            if ziel is None:
                melde(f"videos.yaml {video}: unbekannte ID {k['id']}")
            elif not k.get("fehlt"):
                ziel["video"] = {"video": video, "von": k["von"], "bis": k["bis"]}
                gesetzt += 1
    return gesetzt, {v: {"datei": i["datei"], "beschnitt": i["beschnitt"]} for v, i in alle.items()}


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--ausgabe", default=str(WURZEL / "build" / "daten"))
    ausgabe = Path(ap.parse_args().ausgabe)
    ausgabe.mkdir(parents=True, exist_ok=True)

    ids = korrekturen()
    aikido_gloss = Glossar("begriffe.yaml", bausteine=False)
    disziplinen = []
    for art, gloss in (("aikido", aikido_gloss), ("daitoryu", Glossar("daitoryu-begriffe.yaml", bausteine=True))):
        heft, app = programm(art, ids, gloss)
        disziplinen.append(app)
        (ausgabe / f"{art}.lua").write_text(
            "-- erzeugt von tools/daten.py aus daten/" + art + ".yaml, nicht von Hand bearbeiten\nreturn "
            + lua(heft) + "\n", encoding="utf-8")
    st_heft, st_app = shinki_toho(ids)
    disziplinen.append(st_app)
    (ausgabe / "shinki-toho.lua").write_text(
        "-- erzeugt von tools/daten.py aus daten/shinki-toho.yaml, nicht von Hand bearbeiten\nreturn "
        + lua(st_heft) + "\n", encoding="utf-8")

    mit_video, videos = videostellen(disziplinen)
    app = {"format": FORMAT, "disziplinen": disziplinen, "videos": videos,
           "glossar": {k: v for k, v in yaml.safe_load((DATEN / "begriffe.yaml").read_text(encoding="utf-8")).items()},
           "lexikon": yaml.safe_load((DATEN / "lexikon.yaml").read_text(encoding="utf-8"))}
    (ausgabe / "mitori-daten.json").write_text(json.dumps(app, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    (ausgabe / "fehlende-kanji.txt").write_text(
        "# Namen ohne Kanji (Schlüssel fehlt in daten/begriffe.yaml bzw. daten/daitoryu-begriffe.yaml)\n"
        + "".join(f"{k}\n" for k in sorted(fehlend)), encoding="utf-8")

    techniken = sum(len(d.get("techniken", [])) for d in disziplinen)
    print(f"daten: {techniken} Techniken, {len(st_app['odachi'])} Ô-dachi-Formen, "
          f"{mit_video} mit Videostelle, {len(fehlend)} Namen ohne Kanji → {ausgabe}")
    if fehler:
        print("\n".join(["Fehler in den Daten:"] + [f"  {f}" for f in fehler]), file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
