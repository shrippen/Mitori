import QtQuick
import QtQuick.Controls as QQC2

// Symbol button (bible section 4, `.iconbtn`): the symbol in ink, under the pointer a round
// `chip` ground. Keyboard focus: a 2 px ring in ink.
QQC2.ToolButton {
    id: button

    hoverEnabled: true
    padding: ShinkiStyle.gridSpace * 2
    implicitWidth: Math.max(ShinkiStyle.rowMinSpace, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: ShinkiStyle.rowMinSpace

    QQC2.ToolTip.text: text
    QQC2.ToolTip.visible: hovered && display === QQC2.AbstractButton.IconOnly && text !== ""

    background: Rectangle {
        radius: height / 2
        color: button.checked ? ShinkiStyle.ink : button.hovered || button.down ? ShinkiStyle.chip : "transparent"
        opacity: button.enabled ? 1 : ShinkiStyle.disabledOpacity
        border.width: button.visualFocus ? ShinkiStyle.focusWidth : 0
        border.color: ShinkiStyle.ink
    }
    contentItem: Row {
        spacing: ShinkiStyle.gridSpace

        ShinkiIcon {
            anchors.verticalCenter: parent.verticalCenter
            visible: button.icon.name !== ""
            source: button.icon.name
            color: button.checked ? ShinkiStyle.textOnRed : ShinkiStyle.ink
        }
        QQC2.Label {
            anchors.verticalCenter: parent.verticalCenter
            visible: button.display !== QQC2.AbstractButton.IconOnly && button.text !== ""
            text: button.text
            color: button.checked ? ShinkiStyle.textOnRed : ShinkiStyle.ink
            font.family: ShinkiStyle.uiFamily
            font.pixelSize: ShinkiStyle.uiSize
        }
    }
}
