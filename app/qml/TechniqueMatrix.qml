pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// One grid of the technique map (Programme.techniqueMap, kind "matrix"): techniques in rows,
// attacks in columns, each place coloured by its best level; empty where the technique does not
// answer the attack up to the grade. The technique names stay put while a wide grid scrolls
// sideways. Tab reaches one place per grid, the arrow keys move between the places.
//
//               Katate-dori  Tsuki     ← column names, upright
//   Ikkyo          [3]        [ ]
//   Irimi-nage     [ ]                 ← empty: no such combination
//
// Kante has no labelled matrix yet (ROADMAP.md, Kante-Bedarf); built from its tokens here.
Item {
    id: matrix

    required property var grid
    property real cellSize: KanteStyle.unit(26)
    readonly property int spacing: 2
    readonly property real step: cellSize + spacing
    readonly property var columns: grid.columns || []
    readonly property var rows: grid.rows || []
    // The place Tab reaches (row * columns + column).
    property int current: -1

    // The longest technique name, at most 40 % of the width (longer ones elide).
    // (Reading the metrics' height makes the widths follow their font, set after creation.)
    readonly property real naturalLabelWidth: labelMetrics.height > 0
                                              ? Math.ceil(Math.max(0, ...rows.map(r => labelMetrics.advanceWidth(r.title)))) + KanteStyle.unit(8) : 0
    readonly property real labelWidth: Math.min(width * 0.4, naturalLabelWidth)
    readonly property real headerHeight: headerMetrics.height > 0
                                         ? Math.min(Kirigami.Units.gridUnit * 9,
                                                    Math.ceil(Math.max(0, ...columns.map(c => headerMetrics.advanceWidth(columnTitle(c)))))
                                                    + KanteStyle.unit(6)) : 0

    signal activated(var cell)

    implicitWidth: naturalLabelWidth + columns.length * step
    implicitHeight: headerHeight + rows.length * step + (bar.visible ? bar.height : 0)

    Accessible.role: Accessible.Table
    Accessible.name: grid.title || ""

    onGridChanged: current = firstPlace()
    Component.onCompleted: current = firstPlace()

    function columnTitle(column) {
        return column.extra ? column.title + " · " + column.extra : column.title
    }
    // {} also for a place of the previous grid while the places follow a new one.
    function cellAt(row, column) {
        const r = matrix.rows[row]
        return (r && r.cells[column]) || {}
    }
    function firstPlace() {
        for (let i = 0; i < rows.length * columns.length; i++) {
            if (cellAt(Math.floor(i / columns.length), i % columns.length).ids) {
                return i
            }
        }
        return -1
    }
    // The next filled place from (row, column) in a direction, skipping empty ones; -1 at the edge.
    function nextPlace(row, column, rowStep, columnStep) {
        for (let r = row + rowStep, c = column + columnStep;
             r >= 0 && r < rows.length && c >= 0 && c < columns.length;
             r += rowStep, c += columnStep) {
            if (cellAt(r, c).ids) {
                return r * columns.length + c
            }
        }
        return -1
    }
    // Scrolls the places sideways so that `item` shows.
    function reveal(item) {
        if (item.x < flick.contentX) {
            flick.contentX = item.x
        } else if (item.x + item.width > flick.contentX + flick.width) {
            flick.contentX = item.x + item.width - flick.width
        }
    }

    // The row names' font, as the style gives it to a label.
    QQC2.Label {
        id: probe

        visible: false
    }
    FontMetrics {
        id: labelMetrics

        font: probe.font
    }
    FontMetrics {
        id: headerMetrics

        font: KanteStyle.smallFont
    }

    // The techniques (rows).
    Repeater {
        model: matrix.rows

        QQC2.Label {
            required property var modelData
            required property int index

            y: matrix.headerHeight + index * matrix.step
            width: matrix.labelWidth - KanteStyle.unit(6)
            height: matrix.cellSize
            verticalAlignment: Text.AlignVCenter
            text: modelData.title
            elide: Text.ElideRight
            Accessible.role: Accessible.RowHeader
        }
    }

    Flickable {
        id: flick

        objectName: "mapFlick"
        x: matrix.labelWidth
        width: matrix.width - matrix.labelWidth
        height: matrix.headerHeight + matrix.rows.length * matrix.step
        contentWidth: matrix.columns.length * matrix.step
        contentHeight: height
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        QQC2.ScrollBar.horizontal: bar

        // The attacks (columns), upright, reading upwards from the grid.
        Repeater {
            model: matrix.columns

            Item {
                id: header

                required property var modelData
                required property int index
                readonly property string title: matrix.columnTitle(modelData)

                x: index * matrix.step
                width: matrix.cellSize
                height: matrix.headerHeight - matrix.spacing
                Accessible.role: Accessible.ColumnHeader
                Accessible.name: title

                Text {
                    anchors.centerIn: parent
                    width: parent.height
                    rotation: -90
                    text: header.title
                    elide: Text.ElideRight
                    font: KanteStyle.smallFont
                    color: KanteStyle.mutedTextColor
                }
                HoverHandler {
                    id: headerHover
                }
                QQC2.ToolTip.text: modelData.jp ? title + "  " + modelData.jp : title
                QQC2.ToolTip.visible: headerHover.hovered
                QQC2.ToolTip.delay: 300
            }
        }

        Repeater {
            id: places

            model: matrix.rows.length * matrix.columns.length

            MapPlace {
                id: place

                required property int index
                readonly property int row: Math.floor(index / matrix.columns.length)
                readonly property int column: index % matrix.columns.length
                readonly property var cell: matrix.cellAt(row, column)
                readonly property bool filled: !!cell.ids

                objectName: "mapPlace"
                x: column * matrix.step
                y: matrix.headerHeight + row * matrix.step
                width: matrix.cellSize
                height: matrix.cellSize
                visible: filled
                // Kept while focused: a place of an old grid cannot drop it (Qt warns).
                activeFocusOnTab: activeFocus || (filled && index === matrix.current)
                level: filled && Practice.revision >= 0 ? Practice.bestLevel(cell.ids) : 0
                label: level > 0 ? String(level) : ""
                tip: filled ? qsTr("%1 · %2 · %3: %4").arg(matrix.rows[row].title)
                                                     .arg(matrix.columnTitle(matrix.columns[column]))
                                                     .arg(cell.gradeName).arg(Practice.levelNames[level])
                              + (cell.ids.length > 1 ? " · " + qsTr("%n entries", "", cell.ids.length) : "")
                            : ""
                onActivated: {
                    matrix.current = index
                    matrix.activated(cell)
                }
                onMoved: (down, across) => {
                    const next = matrix.nextPlace(row, column, down, across)
                    if (next >= 0) {
                        places.itemAt(next).forceActiveFocus(Qt.TabFocusReason)
                    }
                }
                onActiveFocusChanged: {
                    if (activeFocus) {
                        matrix.current = index
                        matrix.reveal(place)
                    }
                }
            }
        }
    }

    QQC2.ScrollBar {
        id: bar

        x: flick.x
        y: flick.height
        width: flick.width
        orientation: Qt.Horizontal
        visible: flick.contentWidth > flick.width
        policy: QQC2.ScrollBar.AlwaysOn
    }
}
