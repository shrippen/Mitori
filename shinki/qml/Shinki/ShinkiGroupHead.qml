import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Group head (bible section 5, `.gh`): the title as a red column, the Japanese in inkMuted
// beside it, a line under both.
Item {
    id: head

    property string title: ""
    property string japanese: ""

    implicitHeight: row.implicitHeight + ShinkiStyle.gridSpace + ShinkiStyle.lineWidth
    implicitWidth: row.implicitWidth
    Accessible.role: Accessible.Heading
    Accessible.name: title

    RowLayout {
        id: row

        anchors.left: parent.left
        anchors.right: parent.right
        spacing: ShinkiStyle.gridSpace * 2

        QQC2.Label {
            text: head.title
            color: ShinkiStyle.red
            font.family: ShinkiStyle.narrowFamily
            font.bold: true
            font.capitalization: Font.AllUppercase
            font.pixelSize: ShinkiStyle.columnSize
            font.letterSpacing: ShinkiStyle.columnSize * ShinkiStyle.columnTracking
            elide: Text.ElideRight
            Layout.maximumWidth: head.width
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: head.japanese !== ""
            text: head.japanese
            color: ShinkiStyle.inkMuted
            font.family: ShinkiStyle.japaneseFamily
            font.pixelSize: ShinkiStyle.columnSize
            elide: Text.ElideRight
        }
    }
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: ShinkiStyle.lineWidth
        color: ShinkiStyle.line
    }
}
