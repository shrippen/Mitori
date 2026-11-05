import QtQuick
import QtQuick.Controls as QQC2

// Field (bible section 5, `.qnote input`): paperAlt, 1 px frame, the text in the book face (notes
// are read); with the keyboard focus a 2 px frame in ink. `invalid`: the frame in red.
QQC2.TextField {
    id: field

    property bool invalid: false

    color: ShinkiStyle.ink
    // The placeholder stays inside the box. Material floats its own above a focused field, across the
    // frame and cut off at the edge of a bar: that one is transparent, the Label below is the one seen.
    placeholderTextColor: "transparent"
    selectionColor: ShinkiStyle.red
    selectedTextColor: ShinkiStyle.textOnRed
    font.family: ShinkiStyle.bookFamily
    font.pixelSize: ShinkiStyle.readingSize
    padding: ShinkiStyle.gridSpace * 2
    implicitHeight: Math.max(ShinkiStyle.rowMinSpace, contentHeight + topPadding + bottomPadding)

    QQC2.Label {
        objectName: "fieldHint"
        x: field.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        width: field.width - field.leftPadding - field.rightPadding
        visible: field.placeholderText !== "" && field.text === "" && field.preeditText === ""
        text: field.placeholderText
        elide: Text.ElideRight
        color: ShinkiStyle.inkMuted
        font: field.font
    }

    background: Rectangle {
        implicitWidth: ShinkiStyle.unit(170)        // a field easy to hit, also when empty
        color: ShinkiStyle.paperAlt
        opacity: field.enabled ? 1 : ShinkiStyle.disabledOpacity
        border.width: field.activeFocus ? ShinkiStyle.focusWidth : ShinkiStyle.lineWidth
        border.color: field.invalid ? ShinkiStyle.red : field.activeFocus ? ShinkiStyle.ink : ShinkiStyle.line
    }
}
