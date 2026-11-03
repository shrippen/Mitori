import QtQuick
import QtQuick.Controls as QQC2

// Section label (bible section 5, `.gh`): the column in red (PT Sans Narrow, bold, capitals,
// spaced); `rule` adds the line in `line` under it.
QQC2.Label {
    id: label

    property bool rule: false

    color: ShinkiStyle.red
    font.family: ShinkiStyle.narrowFamily
    font.bold: true
    font.capitalization: Font.AllUppercase
    font.pixelSize: ShinkiStyle.columnSize
    font.letterSpacing: ShinkiStyle.columnSize * ShinkiStyle.columnTracking
    bottomPadding: rule ? ShinkiStyle.gridSpace : 0
    elide: Text.ElideRight
    Accessible.role: Accessible.Heading

    Rectangle {
        visible: label.rule
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: ShinkiStyle.lineWidth
        color: ShinkiStyle.line
    }
}
