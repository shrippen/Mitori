#!/usr/bin/env python3
"""Lehrgänge aus dem öffentlichen Kalender (ICS) als lehrgaenge.js für den Clickdummy."""
import json
import re
import sys
import urllib.request
from pathlib import Path

URL = "https://cal.arianw.de/dav/calendars/shinki/Lehrgaenge?export"


def felder(ics):
    ics = re.sub(r"\r?\n[ \t]", "", ics)  # gefaltete Zeilen
    for block in re.findall(r"BEGIN:VEVENT(.*?)END:VEVENT", ics, re.S):
        block = re.sub(r"BEGIN:VALARM.*?END:VALARM", "", block, flags=re.S)  # die Erinnerung hat eine eigene DESCRIPTION
        f = {}
        for z in block.strip().splitlines():
            k, _, v = z.partition(":")
            name = k.split(";")[0]
            if name == "DESCRIPTION" and "ALTREP" in k:
                v = z.split('":', 1)[1] if '":' in z else v
            f[name] = v.replace("\\,", ",").replace("\\n", " ").replace("\\;", ";").strip()
        yield f


def datum(v):
    m = re.match(r"(\d{4})(\d{2})(\d{2})", v or "")
    return f"{m[1]}-{m[2]}-{m[3]}" if m else None


def main():
    ics = urllib.request.urlopen(URL, timeout=20).read().decode("utf-8")
    out = []
    for f in felder(ics):
        out.append({"titel": f.get("SUMMARY", ""), "von": datum(f.get("DTSTART")), "bis": datum(f.get("DTEND")),
                    "ort": f.get("LOCATION", ""), "info": f.get("DESCRIPTION", ""), "uid": f.get("UID", "")})
    out.sort(key=lambda x: x["von"] or "")
    Path(__file__).with_name("lehrgaenge.js").write_text("window.LEHRGAENGE = " + json.dumps(out, ensure_ascii=False, indent=1) + ";\n", encoding="utf-8")
    print(f"lehrgaenge: {len(out)} Termine", file=sys.stderr)


if __name__ == "__main__":
    main()
