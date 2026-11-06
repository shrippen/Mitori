import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Two equal columns on a tablet sideways (the clickdummy's `.tab .grid2`), else one below the other
// with the page's spacing. Each child sets Layout.fillWidth and Layout.alignment: Qt.AlignTop.
GridLayout {
    required property bool wide

    Layout.fillWidth: true
    columns: wide ? 2 : 1
    uniformCellWidths: wide
    columnSpacing: Kirigami.Units.gridUnit
    rowSpacing: Kirigami.Units.largeSpacing
}
