"""The Android manifest asks for what the app uses (androiddeployqt adds nothing without its
placeholder): the course calendar and the video bundles need the network. Auto Backup leaves out
the bundles (decrypted videos and their keys): hundreds of MB would stop the whole backup."""
import xml.etree.ElementTree as ET
from pathlib import Path

MANIFEST = Path(__file__).resolve().parent.parent / "app" / "android" / "AndroidManifest.xml"
ANDROID = "{http://schemas.android.com/apk/res/android}name"


def test_internet_permission():
    root = ET.parse(MANIFEST).getroot()
    rights = {p.get(ANDROID) for p in root.iter("uses-permission")}
    assert "android.permission.INTERNET" in rights
    assert "android.permission.ACCESS_NETWORK_STATE" in rights      # Wi-Fi only downloads


RES = MANIFEST.parent / "res" / "xml"
TOOLS = "{http://schemas.android.com/apk/res/android}"


def test_backup_without_bundles():
    app = ET.parse(MANIFEST).getroot().find("application")
    # Android 12+ reads dataExtractionRules, older ones fullBackupContent.
    rules = app.get(TOOLS + "dataExtractionRules")
    content = app.get(TOOLS + "fullBackupContent")
    assert rules == "@xml/data_extraction_rules" and content == "@xml/backup_rules"
    for sections, name in ((["cloud-backup", "device-transfer"], "data_extraction_rules.xml"), ([None], "backup_rules.xml")):
        root = ET.parse(RES / name).getroot()
        for section in sections:
            scope = root if section is None else root.find(section)
            excluded = {(e.get("domain"), e.get("path")) for e in scope.iter("exclude")}
            assert ("file", "bundles/") in excluded, (name, section)
