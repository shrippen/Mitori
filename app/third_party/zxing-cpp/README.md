ZXing-C++ 3.1.1, Apache-2.0 (LICENSE), https://github.com/zxing-cpp/zxing-cpp; darin libzueci
(BSD-3-Clause, Kopfzeilen in `core/src/libzueci`) und librscpp (Apache-2.0).

Aus `v3.1.1.tar.gz` (sha256 7286b1e6ade66fe82b7c8208b4595deeb55d6486b410834fdc65702f46650542) nur
`core/` und `LICENSE`, unverändert. Entfernt, weil Mitori nur QR liest: `core/src/{aztec,datamatrix,
maxicode,oned,pdf417,libzint}`, `core/version.rc.in`. `CMakeLists.txt` und diese Datei sind von Mitori.
