import QtQuick
import QtQuick.Layouts
import Mitori

// How well a grade sits: how many of its techniques stand at each rating, as one bar with a
// legend (strongest first, then not rated).
ColumnLayout {
    id: stand

    required property var entries      // the grade's rows (Programme.entries)
    property bool legend: true          // off where one legend serves several bars
    property string name: ""            // the grade's name, for screen readers among several bars
    readonly property var ids: entries.filter(r => r.kind === "entry").map(r => r.id)
    readonly property var counts: Practice.revision >= 0 ? Practice.counts(ids) : [0, 0, 0, 0, 0]
    // Levels 4 … 1, then not rated: the order of the bar and the legend.
    readonly property var order: [4, 3, 2, 1]

    // The statistics name the figure beside the grade already.
    property bool readyShown: true

    spacing: Look.unit(6)
    visible: ids.length > 0

    // How many are exam-ready (the clickdummy's programme page: "3 of 8 exam-ready").
    LookLabel {
        objectName: "readyText"
        Layout.fillWidth: true
        visible: stand.readyShown
        text: qsTr("%1 of %2 exam-ready").arg(stand.counts[4]).arg(stand.ids.length)
        color: Look.mutedTextColor
    }

    LookProgressBar {
        Layout.fillWidth: true
        parts: stand.order.map(l => ({
            value: stand.ids.length ? stand.counts[l] / stand.ids.length : 0,
            color: Look.levelColor(l),
            text: stand.counts[l] + " " + Practice.levelNames[l]
        }))
        Accessible.name: stand.name ? qsTr("Rating of %1").arg(stand.name) : qsTr("Rating of the grade")
    }
    LookLegend {
        Layout.fillWidth: true
        visible: stand.legend
        model: stand.order.concat([0]).map(l => ({
            color: Look.levelColor(l),
            value: stand.counts[l],
            text: Practice.levelNames[l]
        }))
    }
}
