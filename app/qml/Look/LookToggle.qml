import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Controls.impl
import QtQuick.Layouts

// A switch as a button (the player's sound and the screen): a flat field with a frame, the frame
// and the word in the accent while it is on (the clickdummy's `.toggles`). No rounding, no
// colour of its own: the style's.
QQC2.Button {
    id: toggle

    checkable: true
    implicitHeight: Look.unit(34)
    padding: Look.unit(6)
    leftPadding: Look.unit(10)
    rightPadding: Look.unit(10)

    background: Rectangle {
        color: "transparent"
        border.width: toggle.visualFocus ? Look.focusWidth : Look.borderWidth
        border.color: toggle.visualFocus ? Look.focusColor : toggle.checked ? Look.accentColor : Look.ruleColor
    }

    contentItem: RowLayout {
        spacing: Look.unit(6)

        IconImage {
            Layout.preferredWidth: Look.unit(16)
            Layout.preferredHeight: Look.unit(16)
            visible: toggle.icon.name !== ""
            name: toggle.icon.name
            color: toggle.checked ? Look.accentColor : Look.textColor
        }
        LookLabel {
            text: toggle.text
            color: toggle.checked ? Look.accentColor : Look.textColor
            font: Look.defaultFont
        }
    }
}
