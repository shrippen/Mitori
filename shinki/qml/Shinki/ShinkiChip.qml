import QtQuick
import QtQuick.Controls as QQC2

// Grade chip (bible section 5, `.gchip`): the name bold in a 1 px frame; chosen: a red frame and
// 3 px red inside at the bottom (not colour alone: the bar). As KanteChip: `checkable` toggles on
// a tap, `choice` (one of a set the app holds) only reports the tap and keeps the app's binding
// of `checked`. Return and Space act as a tap.
Item {
    id: chip

    property string text: ""
    property bool checkable: false
    property bool checked: false
    property bool choice: false
    readonly property bool hovered: hover.hovered
    signal clicked()

    function activate() {
        if (checkable && !choice) {
            checked = !checked
        }
        clicked()
    }

    implicitWidth: label.implicitWidth + 4 * ShinkiStyle.gridSpace
    implicitHeight: Math.max(ShinkiStyle.chipMinSpace, label.implicitHeight + 2 * ShinkiStyle.gridSpace)
    activeFocusOnTab: true

    Accessible.role: checkable ? Accessible.RadioButton : Accessible.Button
    Accessible.name: text
    Accessible.checked: checked
    Accessible.focusable: true
    Accessible.onPressAction: chip.activate()

    Keys.onReturnPressed: chip.activate()
    Keys.onEnterPressed: chip.activate()
    Keys.onSpacePressed: chip.activate()

    Rectangle {
        anchors.fill: parent
        color: chip.hovered ? ShinkiStyle.chip : ShinkiStyle.paper
        border.width: chip.activeFocus ? ShinkiStyle.focusWidth : ShinkiStyle.lineWidth
        border.color: chip.activeFocus ? ShinkiStyle.ink : chip.checked ? ShinkiStyle.red : ShinkiStyle.line

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
    QQC2.Label {
        id: label

        anchors.centerIn: parent
        text: chip.text
        color: ShinkiStyle.ink
        font.family: ShinkiStyle.uiFamily
        font.bold: true
        font.pixelSize: ShinkiStyle.uiSize
    }

    HoverHandler {
        id: hover
    }
    TapHandler {
        onTapped: chip.activate()
    }
}
