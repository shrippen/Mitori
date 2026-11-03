import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Legend (bible section 5, "Diagramme"): a square of each colour and its words, 11 px inkMuted.
//   model  [{color, value, text}]: "12 sure"
Flow {
    id: legend

    property var model: []

    spacing: ShinkiStyle.gridSpace * 3

    Repeater {
        model: legend.model

        Row {
            required property var modelData

            spacing: ShinkiStyle.gridSpace

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: ShinkiStyle.levelDotSize
                height: width
                color: parent.modelData.color
                border.width: ShinkiStyle.lineWidth
                border.color: ShinkiStyle.line
            }
            QQC2.Label {
                text: (parent.modelData.value !== undefined ? parent.modelData.value + " " : "") + (parent.modelData.text || "")
                color: ShinkiStyle.inkMuted
                font.family: ShinkiStyle.uiFamily
                font.pixelSize: ShinkiStyle.columnSize
                font.features: { "tnum": 1 }
            }
        }
    }
}
