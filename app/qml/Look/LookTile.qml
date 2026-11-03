import QtQuick
import Kante

// A start-menu tile: kanji, name, one line under it; `big` the wide programme tile. Kante:
// KanteOptionTile as a launch tile. Under Shinki +shinki/LookTile.qml (ShinkiTile).
KanteOptionTile {
    property bool big: false

    checkable: false
    autoExclusive: false
}
