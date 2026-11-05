import QtQuick
import QtQuick.Controls as QQC2

// Play button of a list row (bible sections 4 and 5, `.li`): a red circle of 34 px, the symbol
// white. Keyboard focus: a 2 px ring in ink.
QQC2.AbstractButton {
    id: button

    implicitWidth: ShinkiStyle.playListSize
    implicitHeight: implicitWidth
    hoverEnabled: true
    Accessible.name: text

    QQC2.ToolTip.text: text
    QQC2.ToolTip.visible: hovered && text !== ""

    background: Rectangle {
        radius: width / 2
        color: ShinkiStyle.red
        border.width: button.visualFocus ? ShinkiStyle.focusWidth : 0
        border.color: ShinkiStyle.ink
    }
    contentItem: Item {
        ShinkiIcon {
            anchors.centerIn: parent
            name: "media-playback-start"
            color: ShinkiStyle.textOnRed
        }
    }
}
