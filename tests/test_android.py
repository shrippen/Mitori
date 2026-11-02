"""The Android manifest asks for what the app uses (androiddeployqt adds nothing without its
placeholder): the course calendar and the video bundles need the network."""
import xml.etree.ElementTree as ET
from pathlib import Path

MANIFEST = Path(__file__).resolve().parent.parent / "app" / "android" / "AndroidManifest.xml"
ANDROID = "{http://schemas.android.com/apk/res/android}name"


def test_internet_permission():
    root = ET.parse(MANIFEST).getroot()
    rights = {p.get(ANDROID) for p in root.iter("uses-permission")}
    assert "android.permission.INTERNET" in rights
    assert "android.permission.ACCESS_NETWORK_STATE" in rights      # Wi-Fi only downloads
