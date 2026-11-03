import QtQuick
import QtQuick.Controls as QQC2

// Grade chip (bible section 5, `.gchip`): the name bold in a 1 px frame; chosen: a red frame and
// 3 px red inside at the bottom (not colour alone: the bar).
QQC2.AbstractButton {
    id: chip

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    padding: ShinkiStyle.gridSpace * 2
    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: Math.max(ShinkiStyle.chipMinSpace, implicitContentHeight + topPadding + bottomPadding)

    Accessible.role: checkable ? Accessible.RadioButton : Accessible.Button
    Accessible.name: text
    Accessible.checked: checked

    background: Rectangle {
        color: chip.hovered ? ShinkiStyle.chip : ShinkiStyle.paper
        border.width: chip.visualFocus ? ShinkiStyle.focusWidth : ShinkiStyle.lineWidth
        border.color: chip.visualFocus ? ShinkiStyle.ink : chip.checked ? ShinkiStyle.red : ShinkiStyle.line

        Rectangle {
            visible: chip.checked
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: ShinkiStyle.lineWidth
            height: ShinkiStyle.selectedBarWidth
            color: ShinkiStyle.red
        }
    }
    contentItem: QQC2.Label {
        text: chip.text
        color: ShinkiStyle.ink
        font.family: ShinkiStyle.uiFamily
        font.bold: true
        font.pixelSize: ShinkiStyle.uiSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
