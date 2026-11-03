import QtQuick
import QtQuick.Controls as QQC2

// Number field (bible section 5, Zahlenfeld): the field's paperAlt and frame, the number in table
// figures; − and + as framed squares left and right, faded at a limit. With the keyboard focus a
// 2 px frame in ink, `invalid` red. Same interface as QQC2.SpinBox (from, to, value, editable,
// valueModified()).
QQC2.SpinBox {
    id: spin

    property bool invalid: false

    implicitWidth: ShinkiStyle.rowMinSpace * 3
    implicitHeight: ShinkiStyle.rowMinSpace
    padding: 0
    leftPadding: ShinkiStyle.rowMinSpace
    rightPadding: ShinkiStyle.rowMinSpace
    font.family: ShinkiStyle.uiFamily
    font.pixelSize: ShinkiStyle.readingSize
    font.features: { "tnum": 1 }

    background: Rectangle {
        color: ShinkiStyle.paperAlt
        opacity: spin.enabled ? 1 : ShinkiStyle.disabledOpacity
        border.width: spin.activeFocus ? ShinkiStyle.focusWidth : ShinkiStyle.lineWidth
        border.color: spin.invalid ? ShinkiStyle.red : spin.activeFocus ? ShinkiStyle.ink : ShinkiStyle.line
    }

    contentItem: TextInput {
        text: spin.displayText
        font: spin.font
        color: ShinkiStyle.ink
        selectionColor: ShinkiStyle.red
        selectedTextColor: ShinkiStyle.textOnRed
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !spin.editable
        validator: spin.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    down.indicator: Rectangle {
        x: 0
        width: ShinkiStyle.rowMinSpace
        height: spin.height
        color: spin.down.pressed ? ShinkiStyle.chip : "transparent"
        border.width: ShinkiStyle.lineWidth
        border.color: ShinkiStyle.line
        opacity: spin.value > spin.from ? 1 : ShinkiStyle.disabledOpacity

        QQC2.Label {
            anchors.centerIn: parent
            text: "−"
            color: ShinkiStyle.ink
            font.family: ShinkiStyle.uiFamily
            font.bold: true
            font.pixelSize: ShinkiStyle.buttonSize
        }
    }
    up.indicator: Rectangle {
        x: spin.width - width
        width: ShinkiStyle.rowMinSpace
        height: spin.height
        color: spin.up.pressed ? ShinkiStyle.chip : "transparent"
        border.width: ShinkiStyle.lineWidth
        border.color: ShinkiStyle.line
        opacity: spin.value < spin.to ? 1 : ShinkiStyle.disabledOpacity

        QQC2.Label {
            anchors.centerIn: parent
            text: "+"
            color: ShinkiStyle.ink
            font.family: ShinkiStyle.uiFamily
            font.bold: true
            font.pixelSize: ShinkiStyle.buttonSize
        }
    }
}
