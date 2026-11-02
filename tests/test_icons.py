"""Every icon the app names is in its icon theme (app/icons/breeze-mitori, scripts/sync-icons.sh).

Android and the AppImage have no system icon theme: a missing name shows as an empty button.
"""
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent
THEME = ROOT / "app" / "icons" / "breeze-mitori"
# Kirigami's own: the close button of a dialog (DialogHeaderTopContent).
KIRIGAMI = {"window-close", "window-close-symbolic"}


def named_icons():
    names = set(KIRIGAMI)
    for qml in (ROOT / "app" / "qml").glob("*.qml"):
        names |= set(re.findall(r'icon\.name:\s*"([^"]+)"', qml.read_text(encoding="utf-8")))
    return names


def test_every_named_icon_is_in_the_theme():
    present = {svg.stem for svg in THEME.rglob("*.svg")}
    assert sorted(named_icons() - present) == []
