import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// A grid of places with a value in each (the technique map): rows × columns, the value as the
// colour of its level, the labels beside and above it, a legend below. The colours and the faces
// come from the look (Look.levelColor, Look.levelTextColor, Look.smallFont), so the grid belongs
// to the style: Kante's scale under Kante, Shinki's under Shinki. It replaces KanteHeatGrid for
// the pages, which must not show a Kante page under Shinki.
//
//   rowLabels      the names of the rows, left of the grid
//   columnLabels   the names of the columns, above the cells (upright, reading upwards, when
//                  columnLabelsVertical is set)
//   rowTitle, columnTitle   the names of the two axes
//   cells          one list per row; an entry is a number, null (no data) or
//                  {value, label, tip}: `label` replaces the number in the cell, `tip` is the
//                  tooltip
//   minValue, maxValue   the ends of the scale; a value's level is its place on it
//   showValues     the number (or the label) in the cell (default true); the text carries the
//                  value, so the colour is never the only sign
//   showEmpty      a place without data is drawn (the ground, a frame) and takes the focus
//                  (default true); false leaves a blank gap
//   scrollable     the cells scroll sideways under the row labels that stay (default true)
//   fillWidth      the cells are square and as large as the room beside the row labels allows
//   cellSize       the size of a cell
//   rowLabelWidth  the width of the row labels; longer names elide
//   rowLabelFont   the face of the row labels (Look.smallFont by default)
//   columnLabelMaxLength   caps the length of the column labels; they elide
//   columnTips     a tooltip for a column label of its own
//   legendLabels   the word of a level in the legend
//   noDataText     what the legend says about a place without data
//
// A tap on a place and Enter on the focused one fire activated(row, column); the arrow keys
// move the focus.
Item {
    id: root

    property var rowLabels: []
    property var columnLabels: []
    property string rowTitle: ""
    property string columnTitle: ""
    property var cells: []
    property real minValue: 0
    property real maxValue: 4
    property bool showValues: true
    property bool showEmpty: true
    property bool scrollable: true
    property bool fillWidth: false
    property int cellSize: 0
    property int rowLabelWidth: 0
    property font rowLabelFont: Look.smallFont
    property bool columnLabelsVertical: false
    property int columnLabelMaxLength: 40
    property var columnTips: []
    property var legendLabels: []
    property string noDataText: ""

    // The place the keys have on.
    property int currentRow: -1
    property int currentColumn: -1

    signal activated(int row, int column)

    readonly property real gap: Look.unit(4)
    readonly property real cell: cellSize > 0 ? cellSize : Look.unit(26)
    // The current side used for a cell (cellSide is the public alias the test reads; it follows
    // fillWidth when set).
    readonly property real cellSide: root.fillWidth ? root.cellFillWidth : root.cell
    // Cell size when fillWidth is set: the available width / the number of columns minus the
    // gaps between them. Reads cellsFlick.width when the layout has measured; otherwise the
    // parent width, so the very first row gets the right cell size. The column count comes from
    // the first row of cells when column labels are empty (the form grid uses cells, not labels).
    readonly property real cellFillWidthColumnCount: columnLabels.length > 0 ? columnLabels.length
                                                                : (cells[0] ? cells[0].length : 0)
    readonly property real cellFillWidth: {
        if (!fillWidth) {
            return root.cell
        }
        const n = cellFillWidthColumnCount
        if (n === 0) {
            return root.cell
        }
        const usable = (cellsFlick.width > 0 ? cellsFlick.width : root.width) - (n - 1) * root.gap
        return usable > 0 ? Math.max(Look.unit(8), usable / n) : root.cell
    }
    // The room above the cells for the column labels.
    readonly property real labelHeight: columnLabelsVertical ? Look.unit(72) : 0
    readonly property int rows: cells.length

    readonly property int rowCount: cells.length
    readonly property int columnCount: columnLabels.length
    readonly property int columns: columnLabels.length

    // The entry of a place: a number, null (no data) or {value, label, tip}.
    function entryOf(row, column) {
        const line = cells[row]
        return line ? (line[column] !== undefined ? line[column] : null) : null
    }    // A place without data: a gap, when `showEmpty` is off.
    function isEmpty(row, column) {
        const entry = entryOf(row, column)
        return entry === null || entry === undefined
    }
    // The value of a place (0 without data).
    function valueAt(row, column) {
        return entryValue(entryOf(row, column))
    }
    // The cells of the grid, so a page can look one up (the test and the keyboard). A var
    // property cannot be changed in place (the value would be a copy): the map is built anew.
    property var cellItems: ({})

    function registerCell(key, cell) {
        const map = Object.assign({}, cellItems)
        map[key] = cell
        cellItems = map
    }
    function unregisterCell(key) {
        const map = Object.assign({}, cellItems)
        delete map[key]
        cellItems = map
    }
    // The tooltip text of a place (KanteHeatGrid compat).
    function tipOf(row, column) {
        return entryTip(entryOf(row, column))
    }
    // The cell of a place (its item), for the focus and the tests.
    function cellAt(row, column) {
        return cellItems[row + ":" + column] || null
    }
    // The level of a place (0 without data).
    function levelAt(row, column) {
        return levelOf(entryValue(entryOf(row, column)))
    }
    // Whether a place is drawn: a place without data is a gap when `showEmpty` is off.
    function shownAt(row, column) {
        return showEmpty || !isEmpty(row, column)
    }
    // The level of a value: its place on the scale, 0 to 4. A value below the scale is 0.
    function levelOf(value) {
        if (!(value > 0)) {
            return 0
        }
        const span = maxValue - minValue
        if (span <= 0) {
            return 4
        }
        return Math.max(1, Math.min(4, Math.round((value - minValue) / span * 4)))
    }
    function entryValue(entry) {
        if (entry === null || entry === undefined) {
            return 0
        }
        if (typeof entry === "number") {
            return entry
        }
        return entry.value !== undefined ? entry.value : 0
    }
    // The text in a cell: the label of the entry, else its number.
    function entryLabel(entry) {
        if (entry === null || entry === undefined) {
            return ""
        }
        if (typeof entry === "number") {
            return showValues ? String(entry) : ""
        }
        if (entry.label !== undefined) {
            return entry.label
        }
        return showValues ? String(valueOf(entry)) : ""
    }
    function entryTip(entry) {
        if (!entry || typeof entry === "number") {
            return ""
        }
        return entry.tip || ""
    }
implicitHeight: layout.implicitHeight

    ColumnLayout {
        id: layout

        anchors.fill: parent
        spacing: Look.unit(8)

        // The two axes above the grid.
        RowLayout {
            Layout.fillWidth: true
            spacing: Look.unit(8)

            LookLabel {
                Layout.fillWidth: true
                text: root.rowTitle
                font: Look.smallFont
                color: Look.mutedTextColor
                visible: text !== ""
            }
            LookLabel {
                text: root.columnTitle
                font: Look.smallFont
                color: Look.mutedTextColor
                visible: text !== ""
            }
        }

        RowLayout {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.gap

            // The row labels stay beside the cells when they scroll.
            ColumnLayout {
                Layout.preferredWidth: root.rowLabelWidth > 0 ? root.rowLabelWidth : implicitWidth
                Layout.fillHeight: true
                spacing: root.gap

                Item {
                    Layout.preferredHeight: root.labelHeight
                }
                Repeater {
                    model: root.rowLabels

                    LookLabel {
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: root.cell
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        font: root.rowLabelFont
                        color: Look.mutedTextColor
                        text: modelData
                    }
                }
            }

            // The cells, sideways scrollable.
            Flickable {
                id: cellsFlick
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: columnsColumn.width
                contentHeight: columnsColumn.height
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                ColumnLayout {
                    id: columnsColumn

                    spacing: root.gap

                    // The column labels above the cells: upright, reading upwards, when asked.
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.gap

                        Repeater {
                            model: root.columnLabels

                            LookLabel {
                                required property var modelData

                                Layout.preferredWidth: root.cell
                                Layout.preferredHeight: root.labelHeight
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                                maximumLineCount: 1
                                font: root.rowLabelFont
                                color: Look.mutedTextColor
                                text: modelData

                                // Upright, reading upwards from the cells.
                                rotation: root.columnLabelsVertical ? -90 : 0
                                transformOrigin: Item.Center
                            }
                        }
                    }
                    Repeater {
                        model: root.cells

                        RowLayout {
                            id: rowItem

                            required property var modelData
                            required property int index

                            Layout.preferredHeight: root.cell
                            spacing: root.gap

                            Repeater {
                                model: rowItem.modelData

                                Rectangle {
                                    id: cellBox

                                    required property var modelData
                                    required property int index

                                    readonly property bool empty: cellBox.modelData === null
                                                              || cellBox.modelData === undefined
                                    readonly property real level: root.levelOf(root.entryValue(cellBox.modelData))
                                    // A place without data is a gap when `showEmpty` is off.
                                    readonly property bool shown: root.showEmpty || !cellBox.empty

                                    // The cell knows where it is, so a page can look it up.
                                    Component.onCompleted: root.registerCell(rowItem.index + ":" + cellBox.index, cellBox)
                                    Component.onDestruction: root.unregisterCell(rowItem.index + ":" + cellBox.index)

                                    Layout.preferredWidth: root.fillWidth ? root.cellFillWidth : root.cell
                                    Layout.preferredHeight: root.fillWidth ? root.cellFillWidth : root.cell
                                    visible: cellBox.shown
                                    color: cellBox.level > 0 ? Look.levelColor(cellBox.level) : Look.backgroundColor
                                    border.width: root.showEmpty || cellBox.level > 0 ? Look.borderWidth : 0
                                    border.color: Look.ruleColor




                                    // The focused place: the frame of the look.
                                    Rectangle {
                                        anchors.fill: parent
                                        anchors.margins: -Look.borderWidth
                                        visible: root.currentRow === rowItem.index
                                                 && root.currentColumn === cellBox.index
                                        color: "transparent"
                                        border.width: Look.focusWidth
                                        border.color: Look.focusColor
                                    }

                                    LookLabel {
                                        anchors.centerIn: parent
                                        width: parent.width - 2
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                        maximumLineCount: 1
                                        font: Look.monoFont(0)
                                        color: cellBox.level > 0 ? Look.levelTextColor(cellBox.level) : Look.mutedTextColor
                                        text: cellBox.empty ? "" : root.entryLabel(cellBox.modelData)
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !cellBox.empty
                                        onClicked: {
                                            root.currentRow = rowItem.index
                                            root.currentColumn = cellBox.index
                                            root.activated(root.currentRow, root.currentColumn)
                                        }
                                    }

                                    QQC2.ToolTip.visible: !cellBox.empty && root.entryTip(cellBox.modelData) !== ""
                                    QQC2.ToolTip.text: root.entryTip(cellBox.modelData)
                                }
                            }
                        }
                    }
                }
            }
        }

        // The scale below the grid.
        LookLegend {
            Layout.fillWidth: true
            model: {
                const items = []
                for (let level = 0; level <= 4; level++) {
                    items.push({ color: Look.levelColor(level), text: root.legendLabels[level] || String(level) })
                }
                if (root.noDataText !== "") {
                    items.push({ color: "transparent", frame: false, text: root.noDataText })
                }
                return items
            }
        }
    }

    // The keys: the arrows move over the places (over gaps as well), Enter and Space open one.
    // The grid has the focus.
    focus: true
    Keys.onPressed: event => {
        if (root.rows === 0 || root.columns === 0) {
            return
        }
        root.keyAt(event, root.currentRow < 0 ? 0 : root.currentRow, root.currentColumn < 0 ? 0 : root.currentColumn)
    }

    function keyAt(event, row, column) {
        if (root.rows === 0 || root.columns === 0) {
            return
        }
        switch (event.key) {
        case Qt.Key_Left:
            column = Math.max(0, column - 1)
            break
        case Qt.Key_Right:
            column = Math.min(root.columns - 1, column + 1)
            break
        case Qt.Key_Up:
            row = Math.max(0, row - 1)
            break
        case Qt.Key_Down:
            row = Math.min(root.rows - 1, row + 1)
            break
        case Qt.Key_Return:
        case Qt.Key_Enter:
        case Qt.Key_Space:
            root.currentRow = row
            root.currentColumn = column
            root.activated(row, column)
            event.accepted = true
            return
        default:
            return
        }
        root.currentRow = row
        root.currentColumn = column
        event.accepted = true
    }
}
