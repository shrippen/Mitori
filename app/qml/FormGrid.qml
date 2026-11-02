pragma ComponentBehavior: Bound

import QtQuick
import Kante
import Mitori

// Shinki Toho on the technique map (Programme.techniqueMap, kind "forms"): a place per Ô-dachi
// form, numbered in order and coloured by its level. Tab reaches one place, the arrow keys move.
// Placed by hand, not a Grid: the places take their size from the width given.
Item {
    id: forms

    required property var cells
    property int columns: 8
    readonly property real spacing: KanteStyle.unit(4)
    // The place Tab reaches.
    property int current: 0
    readonly property real cellSize: (width - (columns - 1) * spacing) / columns

    signal activated(var cell)

    implicitHeight: Math.ceil(cells.length / columns) * (cellSize + spacing) - spacing

    onCellsChanged: current = 0

    Repeater {
        id: places

        model: forms.cells

        MapPlace {
            id: place

            required property var modelData
            required property int index

            objectName: "formPlace"
            x: (index % forms.columns) * (forms.cellSize + forms.spacing)
            y: Math.floor(index / forms.columns) * (forms.cellSize + forms.spacing)
            width: forms.cellSize
            height: forms.cellSize
            activeFocusOnTab: activeFocus || index === forms.current
            level: Practice.revision >= 0 ? Practice.bestLevel(modelData.ids) : 0
            label: String(modelData.entry.nr)
            tip: qsTr("%1 · %2 · %3: %4").arg(modelData.entry.nr).arg(modelData.entry.title)
                                         .arg(modelData.gradeName).arg(Practice.levelNames[level])
            onActivated: {
                forms.current = index
                forms.activated(modelData)
            }
            onMoved: (down, across) => {
                const next = index + down * forms.columns + across
                if (next >= 0 && next < forms.cells.length) {
                    places.itemAt(next).forceActiveFocus(Qt.TabFocusReason)
                }
            }
            onActiveFocusChanged: {
                if (activeFocus) {
                    forms.current = index
                }
            }
        }
    }
}
