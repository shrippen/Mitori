import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Stand bar (bible section 5, "Stand-Leiste"): the parts [{value 0..1, color, text}] side by side
// in one bar of the slider track's height on `line`; each part's text is its tooltip. Without
// parts, `value` (0..1) fills it in red (a level, as the player's brightness).
Item {
    id: bar

    property var parts: []
    property real value: 0
    readonly property var shown: parts.length > 0 ? parts : [{ value: bar.value, color: ShinkiStyle.red, text: "" }]

    implicitHeight: ShinkiStyle.sliderTrackSize * 2
    implicitWidth: ShinkiStyle.unit(200)
    Accessible.role: Accessible.ProgressBar
    Accessible.description: bar.shown.map(p => p.text).filter(t => !!t).join(", ")

    Rectangle {
        anchors.fill: parent
        color: ShinkiStyle.line
    }
    Row {
        anchors.fill: parent

        Repeater {
            model: bar.shown

            Rectangle {
                id: part

                required property var modelData

                width: Math.round(bar.width * Math.max(0, Math.min(1, modelData.value)))
                height: bar.height
                color: modelData.color

                HoverHandler {
                    id: hover
                }
                QQC2.ToolTip.visible: hover.hovered && !!part.modelData.text
                QQC2.ToolTip.text: part.modelData.text || ""
            }
        }
    }
}
