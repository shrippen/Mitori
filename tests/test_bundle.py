"""Checks of tools/bundle.py: codes, wrapping the key, encrypting files (the app reads the same)."""
import importlib.util
import os
from pathlib import Path

import pytest

nacl = pytest.importorskip("nacl", reason="PyNaCl (python-pynacl) needed for tools/bundle.py")

WURZEL = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("bundle", WURZEL / "tools" / "bundle.py")
bundle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bundle)

KLEIN = {"t": 1, "m": 8}        # Argon2id small for tests (the tool uses t=3, 64 MiB)


def test_code_hat_pruefzeichen():
    for _ in range(200):
        code = bundle.neuer_code()
        assert len(code) == 20
        assert bundle.normalisiert(code) == code
        assert bundle.normalisiert(bundle.gruppiert(code).lower()) == code
        # Ein falsches Zeichen fällt auf.
        i = 7
        falsch = code[:i] + bundle.ALPHABET[(bundle.ALPHABET.index(code[i]) + 1) % 32] + code[i + 1:]
        assert bundle.normalisiert(falsch) is None


def test_code_vertauscht_faellt_auf():
    code = "7KQ2MX9HDV3TAPRWE6Y"
    code += bundle.pruefzeichen(code)
    vertauscht = code[1] + code[0] + code[2:]
    assert bundle.normalisiert(vertauscht) is None


def test_eingabe_nachsichtig():
    code = "0123456789ABCDEFGHJ"
    code += bundle.pruefzeichen(code)
    eingabe = " o" + "i" + "23-4567 89abc defgh j" + code[-1].lower()     # O → 0, I → 1, Bindestriche, Leerzeichen
    assert bundle.normalisiert(eingabe) == code
    assert bundle.normalisiert("0l" + code[2:]) == code                    # L → 1
    assert bundle.normalisiert("zu kurz") is None
    assert bundle.normalisiert("U" * 20) is None


def test_kennung_verraet_den_code_nicht():
    code = bundle.neuer_code()
    k = bundle.kennung(code)
    assert len(k) == 32 and code not in k
    assert k == bundle.kennung(code)


def test_schluessel_umhuellt_und_ausgepackt():
    code = bundle.neuer_code()
    k = os.urandom(32)
    e = bundle.umhuellt(code, "aikido-kyu", k, **KLEIN)
    assert bundle.ausgepackt(code, e) == k
    falsch = bundle.neuer_code()
    with pytest.raises(Exception):
        bundle.ausgepackt(falsch, e)
    anderes = dict(e, bundle="aikido-dan")      # die AD bindet den Schlüssel an sein Bundle
    with pytest.raises(Exception):
        bundle.ausgepackt(code, anderes)


@pytest.mark.parametrize("groesse", [0, 5, bundle.BLOCK, bundle.BLOCK + 1, 2 * bundle.BLOCK + 17])
def test_datei_hin_und_zurueck(tmp_path, groesse):
    klar = tmp_path / "v.mkv"
    klar.write_bytes(os.urandom(groesse))
    k = os.urandom(32)
    g, sha, kg = bundle.verschluesselt(klar, tmp_path / "v.mvb", k)
    assert kg == groesse
    bloecke = max(1, -(-groesse // bundle.BLOCK))
    assert g == 8 + 16 + groesse + 16 * bloecke
    bundle.entschluesselt(tmp_path / "v.mvb", tmp_path / "zurueck.mkv", k)
    assert (tmp_path / "zurueck.mkv").read_bytes() == klar.read_bytes()


def test_abgeschnitten_faellt_auf(tmp_path):
    klar = tmp_path / "v.mkv"
    klar.write_bytes(os.urandom(2 * bundle.BLOCK + 5))
    k = os.urandom(32)
    bundle.verschluesselt(klar, tmp_path / "v.mvb", k)
    roh = (tmp_path / "v.mvb").read_bytes()
    # Ohne den letzten Block: der vorletzte gilt jetzt als letzter, seine AD passt nicht.
    (tmp_path / "kurz.mvb").write_bytes(roh[:8 + 16 + 2 * (bundle.BLOCK + 16)])
    with pytest.raises(Exception):
        bundle.entschluesselt(tmp_path / "kurz.mvb", tmp_path / "x", k)


def test_bundles_kennen_ihre_videos():
    import json
    import subprocess
    import sys
    subprocess.run([sys.executable, str(WURZEL / "tools" / "daten.py")], check=True, capture_output=True)
    videos = json.loads((WURZEL / "build" / "daten" / "mitori-daten.json").read_text(encoding="utf-8"))["videos"]
    alle = [v for b in bundle.definitionen().values() for v in b["videos"]]
    assert sorted(alle) == sorted(videos), "jedes Video in genau einem Bundle"


def test_ids_sind_ordnernamen():
    """Die App nimmt nur Ids aus Kleinbuchstaben, Ziffern und Bindestrichen (BundleShelf::validId)."""
    assert all(bundle.ID.fullmatch(i) for i in bundle.definitionen())
    for falsch in ("..", "/tmp", "a/b", "Aikido", ""):
        assert not bundle.ID.fullmatch(falsch)
