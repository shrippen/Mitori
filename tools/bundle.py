#!/usr/bin/env python3
"""Video-Bundles für Mitori: beschneiden, verschlüsseln, Codes je DVD, Manifest, Upload.

Aufruf aus dem Projektordner:
  python3 tools/bundle.py build aikido-kyu [--videos "../Mitori Videos"] [--version 2] [--ohne-kodieren]
  python3 tools/bundle.py codes aikido-kyu --anzahl 50       Codes als CSV, Einträge im Manifest
  python3 tools/bundle.py sperren 7KQ2M-X9HDV-3TAPR-WE6YC    Eintrag eines Codes aus dem Manifest
  python3 tools/bundle.py pruefen 7KQ2M-X9HDV-3TAPR-WE6YC    öffnet den Schlüssel wie die App
  python3 tools/bundle.py hochladen [--nur-manifest]         als generische Pakete nach Gitea

Geheimes (Inhaltsschlüssel, Codes, die beschnittenen Videos) liegt nie im Repo, sondern im Ordner
MITORI_BUNDLE_GEHEIM (Standard ../bundle-geheim):
  schluessel/<bundle>.key      32 Byte je Bundle, bleibt über Versionen (die Codes gelten weiter)
  codes/<bundle>.csv           Code, Datum, Kennung im Manifest (für den Verband)
  aus/<bundle>/<version>/      die verschlüsselten Dateien zum Hochladen
  manifest.json                das Manifest (öffentlich: verrät weder Codes noch Schlüssel)
  gitea-token                  Token für den Upload (Paket schreiben)

Welche Videos ein Bundle hat: tools/bundles.yaml. Beschnitt wie in mitori-daten.json
(`beschnitt`: Schwarz oben und unten mit den eingebrannten Titeln fällt weg), HEVC hoher Qualität;
Kapitel bleiben, die Zeiten stimmen weiter.

Formate (dieselben in app/store/bundlecrypto.cpp):

  Code      20 Zeichen Crockford-Base32 (0-9 A-Z ohne I L O U), 19 zufällig (95 Bit) und ein
            Prüfzeichen (Luhn mod 32); angezeigt in Gruppen zu fünf: 7KQ2M-X9HDV-3TAPR-WE6YC.
            Eingabe: Groß/klein, Bindestriche und Leerzeichen egal, O gilt als 0, I und L als 1.
  Kennung   hex(BLAKE2b-128("mitori-code:" + Code)): findet den Eintrag, verrät den Code nicht
  Umhüllung Argon2id(Code, Salz 16 Byte, t, m KiB, 1 Spur) → 32 Byte; damit XChaCha20-Poly1305
            über den Inhaltsschlüssel, Nonce 24 Byte, AD "mitori-key:" + Bundle-Id;
            gespeichert: base64(Nonce + Geheimtext + Tag)
  Datei     "MITORIV1" + Präfix (16 Byte), dann Blöcke zu 1 MiB Klartext (der letzte kürzer, bei
            leerer Datei ein leerer), jeder XChaCha20-Poly1305 mit Nonce = Präfix + Index (8 Byte
            LE), AD = "mitori-chunk" + Index (8 Byte LE) + 1 Byte "letzter Block"; Block =
            Geheimtext + Tag (16 Byte). Ein abgeschnittener oder umgestellter Download fällt auf.

  manifest.json
  {"format": 1,
   "bundles": [{"id": "aikido-kyu", "version": 2, "titel": "Aikido · alle Kyu", "videos": [
                  {"video": "aikido/k5", "datei": "aikido-k5.mvb", "groesse": 123, "sha256": "…",
                   "klar": 120, "beschnitten": true}], "hinweis": "neu geschnitten …"}],
   "basis": "https://git.arianw.de/api/packages/shrippen/generic",
   "codes": {"<Kennung>": {"bundle": "aikido-kyu", "salz": "…", "t": 3, "m": 65536, "schluessel": "…"}}}
  Datei-URL: <basis>/mitori-<bundle>/<version>/<datei>; das Manifest: <basis>/mitori-manifest/1/manifest.json

Braucht PyNaCl (libsodium), PyYAML; build braucht ffmpeg.
"""
import argparse
import base64
import csv
import datetime
import hashlib
import json
import os
import re
import secrets
import struct
import subprocess
import sys
import urllib.request
from pathlib import Path

import yaml
from nacl import bindings, pwhash

WURZEL = Path(__file__).resolve().parent.parent
GEHEIM = Path(os.environ.get("MITORI_BUNDLE_GEHEIM", WURZEL.parent / "bundle-geheim"))
BASIS = "https://git.arianw.de/api/packages/shrippen/generic"
MANIFEST_PAKET = ("mitori-manifest", "1", "manifest.json")

ALPHABET = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"   # Crockford-Base32
ZUFALL = 19                                     # Zeichen, dazu ein Prüfzeichen
GRUPPE = 5
MAGIE = b"MITORIV1"
PRAEFIX = 16
BLOCK = 1 << 20                                 # Klartext je Block
TAG = 16
ARGON_T = 3
ARGON_M = 64 * 1024                             # KiB


# ── Codes ──

def pruefzeichen(zeichen):
    """Luhn mod 32 über die Zeichen (Werte im Alphabet): fängt ein falsches und zwei vertauschte
    Nachbarn, außer 0 und Z (Werte 0 und 31)."""
    summe, faktor = 0, 2
    for z in reversed(zeichen):
        wert = faktor * ALPHABET.index(z)
        summe += wert // 32 + wert % 32
        faktor = 1 if faktor == 2 else 2
    return ALPHABET[(32 - summe % 32) % 32]


def normalisiert(eingabe):
    """Die 20 Zeichen eines Codes, oder None, wenn er nicht gültig ist (Länge, Zeichen, Prüfzeichen)."""
    s = eingabe.upper().replace("-", "").replace(" ", "")
    s = s.replace("O", "0").replace("I", "1").replace("L", "1")
    if len(s) != ZUFALL + 1 or any(z not in ALPHABET for z in s):
        return None
    return s if pruefzeichen(s[:-1]) == s[-1] else None


def neuer_code():
    zufall = "".join(secrets.choice(ALPHABET) for _ in range(ZUFALL))
    return zufall + pruefzeichen(zufall)


def gruppiert(code):
    return "-".join(code[i:i + GRUPPE] for i in range(0, len(code), GRUPPE))


def kennung(code):
    return hashlib.blake2b(b"mitori-code:" + code.encode("ascii"), digest_size=16).hexdigest()


def umhuellt(code, bundle, schluessel, t=ARGON_T, m=ARGON_M):
    salz = secrets.token_bytes(16)
    kek = pwhash.argon2id.kdf(32, code.encode("ascii"), salz, opslimit=t, memlimit=m * 1024)
    nonce = secrets.token_bytes(24)
    geheim = bindings.crypto_aead_xchacha20poly1305_ietf_encrypt(schluessel, b"mitori-key:" + bundle.encode(), nonce, kek)
    return {"bundle": bundle, "salz": base64.b64encode(salz).decode(), "t": t, "m": m,
            "schluessel": base64.b64encode(nonce + geheim).decode()}


def ausgepackt(code, eintrag):
    salz = base64.b64decode(eintrag["salz"])
    kek = pwhash.argon2id.kdf(32, code.encode("ascii"), salz, opslimit=eintrag["t"], memlimit=eintrag["m"] * 1024)
    roh = base64.b64decode(eintrag["schluessel"])
    return bindings.crypto_aead_xchacha20poly1305_ietf_decrypt(roh[24:], b"mitori-key:" + eintrag["bundle"].encode(), roh[:24], kek)


# ── Dateien ──

def _ad(index, letzter):
    return b"mitori-chunk" + struct.pack("<Q", index) + bytes([1 if letzter else 0])


def verschluesselt(quelle, ziel, schluessel):
    """Verschlüsselt quelle nach ziel; gibt (Größe, sha256 der verschlüsselten Datei, Klartextgröße)."""
    klar = os.path.getsize(quelle)
    praefix = secrets.token_bytes(PRAEFIX)
    h = hashlib.sha256()
    with open(quelle, "rb") as rein, open(ziel, "wb") as raus:
        def schreib(b):
            raus.write(b)
            h.update(b)
        schreib(MAGIE + praefix)
        index, gelesen = 0, 0
        while True:
            block = rein.read(BLOCK)
            gelesen += len(block)
            letzter = gelesen >= klar
            nonce = praefix + struct.pack("<Q", index)
            schreib(bindings.crypto_aead_xchacha20poly1305_ietf_encrypt(block, _ad(index, letzter), nonce, schluessel))
            index += 1
            if letzter:
                break
    return os.path.getsize(ziel), h.hexdigest(), klar


def entschluesselt(quelle, ziel, schluessel):
    groesse = os.path.getsize(quelle)
    with open(quelle, "rb") as rein, open(ziel, "wb") as raus:
        kopf = rein.read(len(MAGIE) + PRAEFIX)
        if kopf[:len(MAGIE)] != MAGIE:
            raise ValueError("keine Mitori-Videodatei")
        praefix = kopf[len(MAGIE):]
        pos, index = len(kopf), 0
        while True:
            block = rein.read(BLOCK + TAG)
            pos += len(block)
            letzter = pos >= groesse
            nonce = praefix + struct.pack("<Q", index)
            raus.write(bindings.crypto_aead_xchacha20poly1305_ietf_decrypt(block, _ad(index, letzter), nonce, schluessel))
            index += 1
            if letzter:
                break


# ── Manifest und Geheimes ──

def manifest_lesen():
    p = GEHEIM / "manifest.json"
    if p.exists():
        return json.loads(p.read_text(encoding="utf-8"))
    return {"format": 1, "bundles": [], "basis": BASIS, "codes": {}}


def manifest_schreiben(m):
    GEHEIM.mkdir(parents=True, exist_ok=True)
    p = GEHEIM / "manifest.json"
    tmp = p.with_suffix(".tmp")
    tmp.write_text(json.dumps(m, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    tmp.replace(p)


def schluessel(bundle):
    p = GEHEIM / "schluessel" / f"{bundle}.key"
    if not p.exists():
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(secrets.token_bytes(32))
        os.chmod(p, 0o600)
    return p.read_bytes()


# Ids werden Ordnernamen in der App (BundleShelf::validId): nur a-z, 0-9 und Bindestriche.
ID = re.compile(r"[a-z0-9][a-z0-9-]*")


def definitionen():
    liste = yaml.safe_load((WURZEL / "tools" / "bundles.yaml").read_text(encoding="utf-8"))["bundles"]
    for b in liste:
        if not ID.fullmatch(b["id"]):
            sys.exit(f"bundles.yaml: Id „{b['id']}“ nur aus a-z, 0-9 und Bindestrichen")
    return {b["id"]: b for b in liste}


def videodaten():
    p = WURZEL / "build" / "daten" / "mitori-daten.json"
    if not p.exists():
        subprocess.run([sys.executable, str(WURZEL / "tools" / "daten.py")], check=True)
    return json.loads(p.read_text(encoding="utf-8"))["videos"]


# ── Befehle ──

def beschnitten(quelle, ziel, beschnitt):
    """Neu kodiert ohne die schwarzen Balken (HEVC, hohe Qualität), Kapitel und Ton bleiben."""
    filtr = f"crop=iw:{beschnitt['h']}:0:{beschnitt['y']}" if beschnitt.get("h") != beschnitt.get("hoehe") else "null"
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", str(quelle), "-map", "0:v:0", "-map", "0:a?",
                    "-map_chapters", "0", "-vf", filtr, "-c:v", "libx265", "-crf", "20", "-preset", "slow",
                    "-tag:v", "hvc1", "-c:a", "copy", str(ziel)], check=True)


def befehl_build(args):
    b = definitionen()[args.bundle]
    daten = videodaten()
    m = manifest_lesen()
    alt = next((x for x in m["bundles"] if x["id"] == b["id"]), None)
    version = args.version or ((alt["version"] + 1) if alt else 1)
    aus = GEHEIM / "aus" / b["id"] / str(version)
    arbeit = GEHEIM / "arbeit" / b["id"]
    aus.mkdir(parents=True, exist_ok=True)
    arbeit.mkdir(parents=True, exist_ok=True)
    k = schluessel(b["id"])
    videos = []
    for v in b["videos"]:
        info = daten[v]
        quelle = Path(args.videos) / info["datei"]
        datei = v.replace("/", "-") + ".mvb"
        klar = arbeit / (v.replace("/", "-") + ".mkv")
        if args.ohne_kodieren:
            klar = quelle
        elif not klar.exists():
            print(f"bundle: {v} beschneiden …", file=sys.stderr)
            beschnitten(quelle, klar, info["beschnitt"])
        groesse, sha, klargroesse = verschluesselt(klar, aus / datei, k)
        videos.append({"video": v, "datei": datei, "groesse": groesse, "sha256": sha, "klar": klargroesse,
                       "beschnitten": not args.ohne_kodieren})
    eintrag = {"id": b["id"], "version": version, "titel": b["titel"], "videos": videos, "hinweis": args.hinweis or ""}
    m["bundles"] = [x for x in m["bundles"] if x["id"] != b["id"]] + [eintrag]
    m["bundles"].sort(key=lambda x: list(definitionen()).index(x["id"]))
    manifest_schreiben(m)
    print(f"bundle: {b['id']} Version {version}, {len(videos)} Dateien → {aus}", file=sys.stderr)


def befehl_codes(args):
    if args.bundle not in definitionen():
        sys.exit(f"bundle: unbekanntes Bundle {args.bundle}")
    m = manifest_lesen()
    k = schluessel(args.bundle)
    datei = GEHEIM / "codes" / f"{args.bundle}.csv"
    datei.parent.mkdir(parents=True, exist_ok=True)
    neu = not datei.exists()
    with open(datei, "a", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        if neu:
            w.writerow(["code", "datum", "kennung"])
        for _ in range(args.anzahl):
            code = neuer_code()
            m["codes"][kennung(code)] = umhuellt(code, args.bundle, k)
            w.writerow([gruppiert(code), datetime.date.today().isoformat(), kennung(code)])
    os.chmod(datei, 0o600)
    manifest_schreiben(m)
    print(f"bundle: {args.anzahl} Codes für {args.bundle} → {datei}", file=sys.stderr)


def befehl_sperren(args):
    code = normalisiert(args.code)
    if not code:
        sys.exit("bundle: kein gültiger Code")
    m = manifest_lesen()
    if m["codes"].pop(kennung(code), None) is None:
        sys.exit("bundle: Code steht nicht im Manifest")
    manifest_schreiben(m)
    print("bundle: gesperrt (nach dem Hochladen des Manifests wirksam)", file=sys.stderr)


def befehl_pruefen(args):
    code = normalisiert(args.code)
    if not code:
        sys.exit("bundle: kein gültiger Code (Länge, Zeichen oder Prüfzeichen)")
    eintrag = manifest_lesen()["codes"].get(kennung(code))
    if not eintrag:
        sys.exit("bundle: Code steht nicht im Manifest")
    ok = ausgepackt(code, eintrag) == schluessel(eintrag["bundle"])
    print(f"bundle: {gruppiert(code)} öffnet {eintrag['bundle']}: {'ja' if ok else 'NEIN'}")


def hochladen(paket, version, name, daten, token):
    url = f"{BASIS}/{paket}/{version}/{name}"
    kopf = {"Authorization": f"token {token}"}
    try:   # eine Datei gleichen Namens zuerst löschen (das Manifest wird ersetzt)
        urllib.request.urlopen(urllib.request.Request(url, method="DELETE", headers=kopf), timeout=60)
    except urllib.error.HTTPError as e:
        if e.code != 404:
            raise
    urllib.request.urlopen(urllib.request.Request(url, data=daten, method="PUT", headers=kopf), timeout=3600)
    print(f"bundle: hochgeladen {url}", file=sys.stderr)


def befehl_hochladen(args):
    token = (GEHEIM / "gitea-token").read_text().strip()
    m = manifest_lesen()
    if not args.nur_manifest:
        for b in m["bundles"]:
            for v in b["videos"]:
                pfad = GEHEIM / "aus" / b["id"] / str(b["version"]) / v["datei"]
                hochladen(f"mitori-{b['id']}", b["version"], v["datei"], pfad.read_bytes(), token)
    hochladen(*MANIFEST_PAKET, json.dumps(m, ensure_ascii=False, indent=1).encode(), token)


def main():
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = p.add_subparsers(dest="befehl", required=True)
    b = sub.add_parser("build")
    b.add_argument("bundle")
    b.add_argument("--videos", default=str(WURZEL.parent / "Mitori Videos"))
    b.add_argument("--version", type=int)
    b.add_argument("--hinweis", help="was die neue Version bringt (Update-Hinweis in der App)")
    b.add_argument("--ohne-kodieren", action="store_true", help="Originale verschlüsseln (Test)")
    c = sub.add_parser("codes")
    c.add_argument("bundle")
    c.add_argument("--anzahl", type=int, default=1)
    for name in ("sperren", "pruefen"):
        s = sub.add_parser(name)
        s.add_argument("code")
    h = sub.add_parser("hochladen")
    h.add_argument("--nur-manifest", action="store_true")
    args = p.parse_args()
    {"build": befehl_build, "codes": befehl_codes, "sperren": befehl_sperren, "pruefen": befehl_pruefen,
     "hochladen": befehl_hochladen}[args.befehl](args)


if __name__ == "__main__":
    main()
