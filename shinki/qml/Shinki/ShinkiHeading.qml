import QtQuick
import QtQuick.Controls as QQC2

// Heading (bible sections 3 and 5): level 4 and below is the group head (`.gh`), a column in
// red (PT Sans Narrow, bold, capitals, spaced) over a line; levels 1–2 the page title (PT Sans
// bold), level 3 a title in the book face.
QQC2.Label {
    id: heading

    property int level: 1
    readonly property bool groupHead: level >= 4

    color: groupHead ? ShinkiStyle.red : ShinkiStyle.ink
    font.family: groupHead ? ShinkiStyle.narrowFamily : level === 3 ? ShinkiStyle.bookFamily : ShinkiStyle.uiFamily
    font.bold: groupHead || level < 3
    font.weight: level === 3 ? Font.DemiBold : groupHead || level < 3 ? Font.Bold : Font.Normal
    font.capitalization: groupHead ? Font.AllUppercase : Font.MixedCase
    font.letterSpacing: groupHead ? ShinkiStyle.columnSize * ShinkiStyle.columnTracking : 0
    font.pixelSize: groupHead ? ShinkiStyle.columnSize : level === 3 ? ShinkiStyle.sheetTitleSize : ShinkiStyle.pageTitleSize
    bottomPadding: groupHead ? ShinkiStyle.gridSpace : 0
    elide: wrapMode === Text.NoWrap ? Text.ElideRight : Text.ElideNone
    Accessible.role: Accessible.Heading

    Rectangle {
        visible: heading.groupHead
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: ShinkiStyle.lineWidth
        color: ShinkiStyle.line
    }
}
