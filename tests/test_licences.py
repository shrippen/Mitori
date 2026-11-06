"""The README names everything the app ships under a licence of its own (app/third_party)."""
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent


def test_readme_names_what_ships():
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    licence = readme[readme.index("## Lizenz"):]
    missing = [n for n in ("QtKeychain", "ZXing-C++", "libzueci", "librscpp", "Monocypher") if n not in licence]
    assert missing == []
