ZXing-C++ 3.1.1, Apache-2.0 (`LICENSE`), https://github.com/zxing-cpp/zxing-cpp. Darin librscpp
(Apache-2.0, laut SPDX-Zeilen) und libzueci (BSD-3-Clause, nur SPDX-Zeile und Copyright; ein
Lizenztext liegt weder hier noch im libzueci-Repo). Upstream hat keine NOTICE-Datei.

`ATTRIBUTION.txt` und `LICENSE` zeigt die App unter „Über Mitori“. `ATTRIBUTION.txt` ist von Mitori: Copyright aus
den Quellen, BSD-3-Clause im Wortlaut von SPDX, dazu der MIT-Text für den UTF-8-Decoder von Bjoern
Hoehrmann in `core/src/libzueci/zueci.c` (dort gekürzt; hier mit dem üblichen Gewährleistungsabsatz).

Aus dem Release-Asset `zxing-cpp-3.1.1.tar.gz` (sha256 c3c02c29c0b519de7bd4e25b376e606e87f0761befd1282815642a2246613d14,
wie die GitHub-API angibt) nur `core/` und `LICENSE`, byteweise unverändert (geprüft mit `diff -r`). Entfernt, weil Mitori
nur QR liest: `core/src/{aztec,datamatrix,maxicode,oned,pdf417,libzint}`, `core/version.rc.in`. `CMakeLists.txt`,
`ATTRIBUTION.txt` und diese Datei sind von Mitori.
