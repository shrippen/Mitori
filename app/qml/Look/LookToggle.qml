import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Controls.impl

// A switch as a button (the player's sound and the screen): a flat field with a frame, the frame
// and the word in the accent while it is on (the clickdummy's `.toggles`). No rounding, no
// colour of its own: the style's.
QQC2.Button {
    id: toggle

    // On the player's dark ground (the clickdummy's `.tx`): on is a filled white field.
    property bool onStage: false
    readonly property color ink: onStage ? (checked ? Look.stageColor : Look.stageTextColor)
                                         : (checked ? Look.accentColor : Look.textColor)

    checkable: true
    // Material insets the background; its frame then sat around the upper part of the word.
    topInset: 0
    bottomInset: 0
    implicitHeight: Look.unit(34)
    padding: Look.unit(6)
    leftPadding: Look.unit(10)
    rightPadding: Look.unit(10)

    background: Rectangle {
        color: toggle.onStage && toggle.checked ? Look.stageTextColor : "transparent"
        border.width: toggle.visualFocus ? Look.focusWidth : Look.borderWidth
        border.color: toggle.visualFocus ? Look.focusColor
               : toggle.onStage ? Qt.alpha(Look.stageTextColor, 0.45)
               : toggle.checked ? Look.accentColor : Look.ruleColor
    }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        Row {
            id: row

            anchors.centerIn: parent
            spacing: Look.unit(6)

            IconImage {
                anchors.verticalCenter: parent.verticalCenter
                width: Look.unit(16)
                height: Look.unit(16)
                visible: toggle.icon.name !== ""
                name: toggle.icon.name
                color: toggle.ink
            }
            LookLabel {
                anchors.verticalCenter: parent.verticalCenter
                visible: toggle.text !== ""
                text: toggle.text
                color: toggle.ink
                font: Look.defaultFont
            }
        }
    }
}
